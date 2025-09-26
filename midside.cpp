#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

static DaisySeed hw;

// Globals for ADC values (updated in main loop)
float width_target = 1.0f;

// Potentiometer mooth helpers
static inline float linmap(float x, float in_min, float in_max, float out_min, float out_max)
{
    float t = (x - in_min) / (in_max - in_min);
    if(t < 0.f) t = 0.f;
    if(t > 1.f) t = 1.f;
    return out_min + t * (out_max - out_min);
}

float width_smooth = 1.0f;

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    // Potentiometer smoothing (~10 ms time constant @ 96 kHz)
    const float a    = 0.0010417f;
    const float norm = 0.70710678f;

    for(size_t i = 0; i < size; i++)
    {
        width_smooth += a * (width_target - width_smooth);

        float M = in[0][i]; // Mid input is Audio Input 1
        float S = in[1][i]; // Side input is Audio Input 2

        float L = (M + width_smooth * S) * norm;
        float R = (M - width_smooth * S) * norm;

        out[0][i] = L; // Left output is Audio Output 1
        out[1][i] = R; // Right output is Audio Output 2
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();

    // ADC setup: width control
    AdcChannelConfig adc_cfg[1];
    adc_cfg[0].InitSingle(hw.GetPin(15)); // Width control pot to ADC 0
    hw.adc.Init(adc_cfg, 1);
    hw.adc.Start();

    // Audio config: 96kHz, blocksize = 4
    hw.SetAudioBlockSize(4);
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);
    hw.StartAudio(AudioCallback);

    while(1)
    {
        // To reduce high frequency noise only update width value at ~100 Hz (every 10 ms)
        width_target = linmap(hw.adc.GetFloat(0), 0.f, 1.f, 0.f, 2.0f);

        System::Delay(10);
    }
    /* For noise:
    Connect one 1000uf electrolytic cap and one 10nf film cap between ground and VIN. 
    Connect one 1000uf cap between ground and 3V3.*/
}