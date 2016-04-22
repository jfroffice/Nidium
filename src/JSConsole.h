#ifndef serverjsconsole_h__
#define serverjsconsole_h__

#include <Binding/JSExposer.h>

namespace Nidium {
namespace Server {

class JSconsole : public Nidium::Binding::JSExposer<JSconsole>
{
  public:
    static void registerObject(JSContext *cx);
};

} // namespace Nidium
} // namespace Server

#endif

