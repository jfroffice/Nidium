/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#ifndef core_events_h__
#define core_events_h__

#include "Core/Hash.h"
#include "Core/Messages.h"

namespace Nidium {
namespace Core {

#define NIDIUM_EVENTS_MESSAGE_BITS(id) ((1 << 31) | id)
#define NIDIUM_EVENT(classe, event) (NIDIUM_EVENTS_MESSAGE_BITS(classe::event) | (classe::EventID << 16))

/*
    Implementation Note :

    Children of Events must define a static
    const property "static const uint8_t EventID" with an unique 8bit identifier
*/

class Events
{
public:
    void addListener(Messages *listener) {
        m_Listeners.set(reinterpret_cast<uint64_t>((static_cast<Messages *>(listener))), listener);

        listener->listenFor(this, true);
    }
    void removeListener(Messages *listener, bool propagate = true) {
        m_Listeners.erase(reinterpret_cast<uint64_t>(listener));

        if (propagate) {
            listener->listenFor(this, false);
        }
    }

    template <typename T>
    bool fireEventSync(typename T::Events event, const Args &args) {
        return this->fireEventImpl<T>(event, args, Events::kPropagationMode_Sync);
    }

    template <typename T>
    bool fireEvent(typename T::Events event, const Args &args,
        bool forceAsync = false) {

        return this->fireEventImpl<T>(event, args, forceAsync ? Events::kPropagationMode_Async : Events::kPropagationMode_Auto);
    }

    virtual ~Events() {
        ape_htable_item_t *item;

        for (item = m_Listeners.accessCStruct()->first; item != NULL; item = item->lnext) {
            Messages *receiver = static_cast<Messages *>(item->content.addrs);

            receiver->listenFor(this, false);
        }
    }

private:
    enum PropagationMode {
        kPropagationMode_Auto,
        kPropagationMode_Sync,
        kPropagationMode_Async
    };

    Hash64<Messages *> m_Listeners;

    template <typename T>
    bool fireEventImpl(typename T::Events event, const Args &args,
        PropagationMode propagation = Events::kPropagationMode_Auto) {

        ape_htable_item_t *item;

        for (item = m_Listeners.accessCStruct()->first; item != NULL; item = item->lnext) {
            Messages *receiver = static_cast<Messages *>(item->content.addrs);

            SharedMessages::Message *msg =
                new SharedMessages::Message(NIDIUM_EVENTS_MESSAGE_BITS(event) | (T::EventID << 16));

            msg->m_Args[0].set(static_cast<T *>(this));

            for (int i = 0; i < args.size(); i++) {
                msg->m_Args[i+1].set(args[i].toInt64());
            }
            msg->m_Priv = 0;

            if (propagation == Events::kPropagationMode_Sync) {
                receiver->postMessageSync(msg);
            } else {
                receiver->postMessage(msg, propagation == Events::kPropagationMode_Async ? true : false);
            }
#if 0
            /* TODO FIX : Use after free here */
            /* stop propagation */
            if (msg->priv) {
                return false;
            }
#endif
        }

        return true;
    }
};

} // namespace Core
} // namespace Nidium

#endif

