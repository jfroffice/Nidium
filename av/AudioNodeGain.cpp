/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#include "AudioNodeGain.h"
#include "processor/Gain.h"

namespace Nidium {
namespace AV {

AudioNodeGain::AudioNodeGain(int inCount, int outCount, Audio *audio)
    : AudioNodeProcessor(inCount, outCount, audio)
{
    m_Args[0] = new ExportsArgs("gain", DOUBLE, GAIN, AudioNodeGain::argCallback);
    m_GainProcessor = new AudioProcessorGain();

    this->setProcessor(0, m_GainProcessor);
    this->setProcessor(1, m_GainProcessor);
}

void AudioNodeGain::argCallback(AudioNode *node, int id, void *tmp, int size)
{
    AudioNodeGain *thiz = static_cast<AudioNodeGain*>(node);
    switch (id) {
        case GAIN:
            thiz->m_GainProcessor->setGain(*static_cast<double*>(tmp));
        break;
    }
}

} // namespace AV
} // namespace Nidium

