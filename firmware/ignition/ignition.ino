#include <Arduino.h>
#include <driver/i2s_std.h>
#include <freertos/queue.h>
#include "core.h"
#include "presets.h"
#include "board_config.h"

#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error This wiring profile is for ESP32-S3 only.
#endif

using namespace ignition;
struct AudioCommand {uint32_t time;size_t preset;float rpm,load,volume;};
struct AudioStats {uint32_t blocks=0,renderMaxUs=0,renderLate=0,writeErrors=0;};
static QueueHandle_t commands=nullptr,statistics=nullptr;
static i2s_chan_handle_t tx=nullptr;
static Controller controller;
static Synth synth;
static size_t currentPreset=0;
static bool ready=false;
static int triggerRaw=0;
static float filteredThrottle=0;

static void audioTask(void*) {
  float mono[board::blockFrames];int32_t stereo[board::blockFrames*2];
  AudioCommand command{};AudioStats stats;size_t lastPreset=kPresetCount;
  for(;;){
    xQueuePeek(commands,&command,0);
    const uint32_t started=micros();
    if(command.preset!=lastPreset){lastPreset=command.preset;synth.setPatch(kPatches[lastPreset]);}
    // If the input task stops publishing, ramp to silence rather than hold a rev.
    const bool fresh=uint32_t(millis()-command.time)<250;
    synth.process(mono,board::blockFrames,command.rpm,command.load,fresh&&!stats.writeErrors?command.volume:0);
    for(int i=0;i<board::blockFrames;i++)stereo[2*i]=stereo[2*i+1]=pcm32(mono[i]);
    const uint32_t elapsed=micros()-started;
    stats.renderMaxUs=std::max(stats.renderMaxUs,elapsed);
    if(elapsed>board::blockFrames*1000000u/sampleRate)stats.renderLate++;
    size_t written=0;
    if(i2s_channel_write(tx,stereo,sizeof(stereo),&written,50)!=ESP_OK||written!=sizeof(stereo)){
      stats.writeErrors++;vTaskDelay(pdMS_TO_TICKS(5));
    }
    stats.blocks++;xQueueOverwrite(statistics,&stats);
  }
}

static bool beginAudio(){
  i2s_chan_config_t channel=I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1,I2S_ROLE_MASTER);
  channel.dma_desc_num=4;channel.dma_frame_num=board::blockFrames;channel.auto_clear=true;
  if(i2s_new_channel(&channel,&tx,nullptr)!=ESP_OK)return false;
  i2s_std_config_t config{};
  config.clk_cfg=I2S_STD_CLK_DEFAULT_CONFIG(sampleRate);
  config.slot_cfg=I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT,I2S_SLOT_MODE_STEREO);
  config.gpio_cfg.mclk=I2S_GPIO_UNUSED;config.gpio_cfg.din=I2S_GPIO_UNUSED;
  config.gpio_cfg.bclk=static_cast<gpio_num_t>(board::bclk);
  config.gpio_cfg.ws=static_cast<gpio_num_t>(board::wordSelect);
  config.gpio_cfg.dout=static_cast<gpio_num_t>(board::dataOut);
  if(i2s_channel_init_std_mode(tx,&config)!=ESP_OK){i2s_del_channel(tx);tx=nullptr;return false;}
  if(i2s_channel_enable(tx)!=ESP_OK){i2s_del_channel(tx);tx=nullptr;return false;}
  commands=xQueueCreate(1,sizeof(AudioCommand));statistics=xQueueCreate(1,sizeof(AudioStats));
  AudioCommand initial{millis(),0,750,0,0};
  if(commands)xQueueOverwrite(commands,&initial);
  if(commands&&statistics&&xTaskCreatePinnedToCore(audioTask,"ignition-audio",8192,nullptr,4,nullptr,0)==pdPASS)return true;
  if(commands){vQueueDelete(commands);commands=nullptr;}
  if(statistics){vQueueDelete(statistics);statistics=nullptr;}
  i2s_channel_disable(tx);i2s_del_channel(tx);tx=nullptr;return false;
}

void setup(){
  Serial.begin(115200); // UART socket; no wait for a connected terminal
  for(int pin:{board::gearUp,board::gearDown,board::brake,board::ignitionSwitch,board::boost,board::character})pinMode(pin,INPUT_PULLUP);
  pinMode(board::throttle,INPUT_PULLDOWN);analogReadResolution(12);analogSetPinAttenuation(board::throttle,ADC_11db);
  controller.drive.configure(kPatches[0]);
  ready=beginAudio();
  rgbLedWrite(board::rgb,ready?0:20,0,ready?5:0);
  Serial.printf("Ignition handheld 0.2 | ESP32-S3 | detected_flash=%u | PSRAM unused | audio=%s\n",ESP.getFlashChipSize(),ready?"ready":"FAILED");
  Serial.println("Release trigger. Switch ignition OFF, then ON. UART: 115200. No automatic boot sound.");
}

void loop(){
  static TickType_t wake=xTaskGetTickCount();
  static uint32_t lastReport=0,lastLed=0;
  static AudioStats stats;
  const uint32_t now=millis();
  if(!ready){if(now-lastLed>=250){lastLed=now;rgbLedWrite(board::rgb,(now/250)%2?20:0,0,0);}delay(10);return;}
  triggerRaw=analogRead(board::throttle);
  float throttle=normalizeTrigger(triggerRaw,board::triggerReleased,board::triggerFull);
  if(board::triggerReversed)throttle=1-throttle;
  filteredThrottle+=(throttle-filteredThrottle)*.45f;
  Inputs inputs;inputs.throttle=filteredThrottle<.01f?0:filteredThrottle;
  inputs.up=digitalRead(board::gearUp)==LOW;inputs.down=digitalRead(board::gearDown)==LOW;inputs.brake=digitalRead(board::brake)==LOW;
  inputs.ignition=digitalRead(board::ignitionSwitch)==LOW;inputs.boost=digitalRead(board::boost)==LOW;inputs.character=digitalRead(board::character)==LOW;
  controller.update(now,inputs);
  if(controller.nextCharacter){currentPreset=(currentPreset+1)%kPresetCount;controller.drive.configure(kPatches[currentPreset]);}
  xQueuePeek(statistics,&stats,0);
  if(stats.writeErrors)controller.drive.stop();
  const AudioCommand command{now,currentPreset,controller.drive.rpm,controller.drive.load,controller.drive.state!=Drive::Off?board::volume:0};
  xQueueOverwrite(commands,&command);
  if(now-lastLed>=50){
    lastLed=now;
    if(stats.writeErrors||controller.drive.overRev)rgbLedWrite(board::rgb,20,0,0);
    else if(controller.drive.state==Drive::Off)rgbLedWrite(board::rgb,0,0,3);
    else if(controller.drive.state==Drive::Cranking)rgbLedWrite(board::rgb,12,8,0);
    else if(controller.boostOn)rgbLedWrite(board::rgb,12,0,20);
    else rgbLedWrite(board::rgb,controller.drive.limiter?20:0,5+int(15*controller.drive.rpm/kPatches[currentPreset].redline),0);
  }
  if(now-lastReport>=1000){
    lastReport=now;
    Serial.printf("preset=%s gear=%d rpm=%.0f speed=%.1f throttle=%.2f adc=%d blocks=%u render_max_us=%u render_late=%u write_errors=%u heap=%u\n",kPresetNames[currentPreset],controller.drive.gear,controller.drive.rpm,controller.drive.speed,inputs.throttle,triggerRaw,stats.blocks,stats.renderMaxUs,stats.renderLate,stats.writeErrors,ESP.getFreeHeap());
  }
  vTaskDelayUntil(&wake,pdMS_TO_TICKS(10));
}
