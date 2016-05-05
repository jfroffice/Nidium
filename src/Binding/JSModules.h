/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#ifndef binding_jsmodules_h__
#define binding_jsmodules_h__

#include <string.h>

#include <jsapi.h>
#include <jspubtd.h>

#include "Core/Hash.h"


namespace Nidium {
namespace Binding {

#define NIDIUM_MODULES_PATHS_COUNT 2

class JSModules;

// {{{ JSModule
class JSModule
{
    public:
        JSModule(JSContext *cx, JSModules *modules, JSModule *parent, const char *name);

        enum ModuleType {
            kModuleType_None,
            kModuleType_JS,
            kModuleType_Nidium,
            kModuleType_JSON
        };

        char *m_AbsoluteDir;
        char *m_FilePath;
        char *m_Name;
        int m_ModuleType;
        bool m_Cached;

        JS::Heap<JSObject *> m_Exports;

        JSModule *m_Parent;
        JSModules *m_Modules;

        bool init();
        bool initJS();
        bool initMain();
        bool initNidium();

        JS::Value require(char *name);

        ~JSModule();
    private:
        JSContext *m_Cx;

        JS::Value load(JS::Value &scope);
};
// }}}

// {{{ JSModules
class JSModules
{
    public:
        JSModules(JSContext *cx) : m_Main(NULL), m_TopDir("/"), m_Cx(cx)
        {
            m_Paths[0] = static_cast<const char *>("modules");
            m_Paths[1] = static_cast<const char *>("node_modules");
            memset(&m_EnvPaths[0], '\0', sizeof(m_EnvPaths));
        }

        ~JSModules()
        {
            for (int i = 0; m_EnvPaths[i] != NULL; i++) {
                free(m_EnvPaths[i]);
            }
            delete m_Main;
        }

        JSModule *m_Main;
        const char *m_TopDir;

        void add(JSModule *module)
        {
            m_Cache.set(module->m_FilePath, module);
            module->m_Cached = true;
        }

        void remove(JSModule *module)
        {
            m_Cache.erase(module->m_FilePath);
        }

        JSModule *find(JSModule *module)
        {
            return m_Cache.get(module->m_FilePath);
        }

        void setPath(const char *topDir)
        {
            m_TopDir = topDir;
        }

        bool init();
        bool init(JSModule *module);

        static char *FindModulePath(JSModule *parent, JSModule *module);
        static bool GetFileContent(const char *file, char **content, size_t *size);
    private:
        Nidium::Core::Hash<JSModule *> m_Cache;
        const char *m_Paths[2];
        char *m_EnvPaths[64];
        JSContext *m_Cx;

        bool initJS(JSModule *cmodule);

        static std::string FindModuleInPath(JSModule *module, const char *path);
        static bool LoadDirectoryModule(std::string &dir);

        static void DirName(std::string &source);
};
// }}}

} // namespace Binding
} // namespace Nidium

#endif

