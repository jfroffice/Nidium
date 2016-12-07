/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#ifndef frontend_nml_h__
#define frontend_nml_h__

#include <ape_netlib.h>

#include <rapidxml.hpp>

#include "Frontend/Assets.h"
#include "Binding/JSStream.h"

#define XML_VP_MAX_WIDTH 8000
#define XML_VP_MAX_HEIGHT 8000
#define XML_VP_DEFAULT_WIDTH 980
#define XML_VP_DEFAULT_HEIGHT 700

namespace Nidium {
namespace Frontend {

class NML;

typedef void (*NMLLoadedCallback)(void *arg);

typedef struct _NMLTag
{
    const char *id;
    const char *tag;

    struct
    {
        const unsigned char *data;
        size_t len;
        bool isBinary;
    } content;
} NMLTag;

class NML : public Core::Messages
{
public:
    explicit NML(_ape_global *net);
    ~NML();

    typedef enum {
        NIDIUM_XML_OK,
        NIDIUM_XML_ERR_VIEWPORT_SIZE,
        NIDIUM_XML_ERR_IDENTIFIER_TOOLONG,
        NIDIUM_XML_ERR_META_MISSING,
        NIDIUM_XML_ERR_META_MULTIPLE
    } nidium_xml_ret_t;

    typedef nidium_xml_ret_t (NML::*tag_callback)(rapidxml::xml_node<> &node);

    void onMessage(const Core::SharedMessages::Message &msg);
    void loadFile(const char *filename, NMLLoadedCallback cb, void *arg);

    void loadDefaultItems(Assets *assets);
    nidium_xml_ret_t loadAssets(rapidxml::xml_node<> &node);
    nidium_xml_ret_t loadMeta(rapidxml::xml_node<> &node);
    nidium_xml_ret_t loadLayout(rapidxml::xml_node<> &node);

    void onAssetsItemReady(Assets::Item *item);
    void onAssetsBlockReady(Assets *asset);
    void onGetContent(const char *data, size_t len);

    const char *getMetaTitle() const
    {
        return m_Meta.title;
    }
    const char *getIdentifier() const
    {
        return m_Meta.identifier;
    }
    int getMetaWidth() const
    {
        return m_Meta.size.width;
    }
    int getMetaHeight() const
    {
        return m_Meta.size.height;
    }

    rapidxml::xml_node<> *getLayout() const
    {
        return m_Layout;
    }

    JSObject *getJSObjectLayout() const
    {
        return m_JSObjectLayout;
    }

    JSObject *buildLayoutTree(rapidxml::xml_node<> &node);

    void setNJS(Binding::NidiumJS *js);

    /*
        str must be null-terminated.
        str is going to be modified
    */
    static JSObject *BuildLST(JSContext *cx, char *str);

private:
    static JSObject *BuildLSTFromNode(JSContext *cx,
                                      rapidxml::xml_node<> &node);

    bool loadData(char *data, size_t len, rapidxml::xml_document<> &doc);
    void addAsset(Assets *);
    ape_global *m_Net;
    IO::Stream *m_Stream;

    /* Define callbacks for tags in <application> */
    struct _nml_tags
    {
        const char *str;
        tag_callback cb; // Call : (this->*cb)()
        bool unique;
    } m_NmlTags[4] = { { "assets", &NML::loadAssets, false },
                       { "meta", &NML::loadMeta, true },
                       { "layout", &NML::loadLayout, true },
                       { NULL, NULL, false } };

    uint32_t m_nAssets;

    Binding::NidiumJS *m_Njs;

    struct
    {
        char *identifier;
        char *title;
        bool loaded;
        struct
        {
            int width;
            int height;
        } size;
    } m_Meta;

    struct
    {
        Assets **list;
        uint32_t allocated;
        uint32_t size;
    } m_AssetsList;
    NMLLoadedCallback m_Loaded;
    void *m_LoadedArg;

    rapidxml::xml_node<> *m_Layout;
    JS::Heap<JSObject *> m_JSObjectLayout;

    bool m_DefaultItemsLoaded;
    bool m_LoadFramework;
    bool m_LoadHTML5;
};

} // namespace Frontend
} // namespace Nidium

#endif
