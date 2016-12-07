/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#include "AudioNodeDelay.h"

namespace Nidium {
namespace AV {

AudioNodeDelay::AudioNodeDelay(int inCount, int outCount, Audio *audio)
    : AudioNodeProcessor(inCount, outCount, audio)
{
    m_Args[0]
        = new ExportsArgs("wet", DOUBLE, WET, AudioNodeDelay::argCallback);
    m_Args[1]
        = new ExportsArgs("delay", INT, DELAY, AudioNodeDelay::argCallback);
    m_Args[2] = new ExportsArgs("feedback", DOUBLE, FEEDBACK,
                                AudioNodeDelay::argCallback);

    m_DelayProcessor = new AudioProcessorDelay(
        m_Audio->m_OutputParameters->m_SampleRate, 2000);

    this->setProcessor(0, m_DelayProcessor);
    this->setProcessor(1, m_DelayProcessor);
}

void AudioNodeDelay::argCallback(AudioNode *node, int id, void *tmp, int size)
{
    AudioNodeDelay *thiz = static_cast<AudioNodeDelay *>(node);
    switch (id) {
        case DELAY:
            thiz->m_DelayProcessor->setDelay(*static_cast<int *>(tmp));
            break;
        case WET:
            thiz->m_DelayProcessor->setWet(*static_cast<double *>(tmp));
            break;
        case FEEDBACK:
            thiz->m_DelayProcessor->setFeedback(*static_cast<double *>(tmp));
            break;
    }
}

} // namespace AV
} // namespace Nidium
