#include "NativeNML.h"
#include "NativeJS.h"

#include <string.h>
#include "NativeHash.h"
#include "NativeStream.h"
#include "NativeJS.h"
#include "NativeJSWindow.h"
#include "NativeJSDocument.h"
#include <jsapi.h>

NativeNML::NativeNML(ape_global *net) :
    net(net), stream(NULL), relativePath(NULL), nassets(0),
    njs(NULL), m_Layout(NULL), m_JSObjectLayout(NULL)
{
    assetsList.size = 0;
    assetsList.allocated = 4;

    assetsList.list = (NativeAssets **)malloc(sizeof(NativeAssets *) * assetsList.allocated);

    this->meta.title = NULL;
    this->meta.size.width = 0;
    this->meta.size.height = 0;
    this->meta.identifier = NULL;
}

NativeNML::~NativeNML()
{
    if (m_JSObjectLayout) {
        JS_RemoveObjectRoot(this->njs->cx, &m_JSObjectLayout);
        m_JSObjectLayout = NULL;
    }
    if (relativePath) {
        if (this->njs) {
            this->njs->setPath(NULL);
        }
        free(relativePath);
    }
    if (stream) {
        delete stream;
    }
    for (int i = 0; i < assetsList.size; i++) {
        delete assetsList.list[i];
    }
    free(assetsList.list);
    if (this->meta.title) {
        free(this->meta.title);
    }
}

void NativeNML::setNJS(NativeJS *js)
{
    this->njs = js;
    this->njs->setPath(this->relativePath);
}

void NativeNML::loadFile(const char *file, NMLLoadedCallback cb, void *arg)
{
    this->loaded = cb;
    this->loaded_arg = arg;

    this->relativePath = NativeStream::resolvePath(file, NativeStream::STREAM_RESOLVE_PATH);

    stream = new NativeStream(this->net, file);
    stream->setDelegate(this);
    stream->getContent();
}

void NativeNML::onAssetsItemReady(NativeAssets::Item *item)
{
    NMLTag tag;
    memset(&tag, 0, sizeof(NMLTag));
    size_t len = 0;
    const unsigned char *data = item->get(&len);

    tag.tag = item->getTagName();
    tag.id = item->getName();
    tag.content.data = data;
    tag.content.len = len;
    tag.content.isBinary = false;

    switch(item->fileType) {
        case NativeAssets::Item::ITEM_SCRIPT:
        {
            size_t len = 0;
            const unsigned char *data = item->get(&len);
            njs->LoadScriptContent((const char *)data, len, item->getName());

            break;
        }
        case NativeAssets::Item::ITEM_NSS:
        {
            NativeJSdocument *jdoc = NativeJSdocument::getNativeClass(njs->cx);
            if (jdoc == NULL) {
                return;
            }
            jdoc->populateStyle(njs->cx, (const char *)data,
                len, item->getName());
            break;
        }
        default:
            break;
    }
    /* TODO: allow the callback to change content ? */

    NativeJSwindow::getNativeClass(njs)->assetReady(tag);
}

static void NativeNML_onAssetsItemRead(NativeAssets::Item *item, void *arg)
{
    class NativeNML *nml = (class NativeNML *)arg;

    nml->onAssetsItemReady(item);
}

void NativeNML::onAssetsBlockReady(NativeAssets *asset)
{
    this->nassets--;

    if (this->nassets == 0) {
        NativeJSwindow::getNativeClass(njs)->onReady(m_JSObjectLayout);
        JS_RemoveObjectRoot(njs->cx, &m_JSObjectLayout);
        m_JSObjectLayout = NULL;
    }
}

static void NativeNML_onAssetsReady(NativeAssets *assets, void *arg)
{
    class NativeNML *nml = (class NativeNML *)arg;

    nml->onAssetsBlockReady(assets);
}

void NativeNML::addAsset(NativeAssets *asset)
{
    this->nassets++;
    if (assetsList.size == assetsList.allocated) {
        assetsList.allocated *= 2;
        assetsList.list = (NativeAssets **)realloc(assetsList.list,
            sizeof(NativeAssets *) * assetsList.allocated);
    }

    assetsList.list[assetsList.size] = asset;

    assetsList.size++;
}

NativeNML::nidium_xml_ret_t NativeNML::loadMeta(rapidxml::xml_node<> &node)
{
    using namespace rapidxml;
    for (xml_node<> *child = node.first_node(); child != NULL;
        child = child->next_sibling())
    {
        if (strncasecmp(child->name(), "title", 5) == 0) {
            if (this->meta.title)
                free(this->meta.title);

            this->meta.title = (char *)malloc(sizeof(char) *
                (child->value_size() + 1));

            memcpy(this->meta.title, child->value(), child->value_size());
            this->meta.title[child->value_size()] = '\0';

        } else if (strncasecmp(child->name(), "viewport", 8) == 0) {
            char *pos;
            if ((pos = (char *)memchr(child->value(), 'x',
                child->value_size())) == NULL) {

                return NIDIUM_XML_ERR_VIEWPORT_SIZE;
            }
            *pos = '\0';
            int width = atoi(child->value());
            if (width < 1 || width > XML_VP_MAX_WIDTH) {
                return NIDIUM_XML_ERR_VIEWPORT_SIZE;
            }
            this->meta.size.width = width;
            *(char *)(child->value()+child->value_size()) = '\0';

            int height = atoi(pos+1);

            if (height < 0 || height > XML_VP_MAX_HEIGHT) {
                return NIDIUM_XML_ERR_VIEWPORT_SIZE;
            }
            this->meta.size.height = height;
        } else if (strncasecmp(child->name(), "identifier", 10) == 0) {
            if (child->value_size() > 128) {
                return NIDIUM_XML_ERR_IDENTIFIER_TOOLONG;
            }
            if (this->meta.identifier)
                free(this->meta.identifier);

            this->meta.identifier = (char *)malloc(sizeof(char) *
                (child->value_size() + 1));

            memcpy(this->meta.identifier, child->value(), child->value_size());
            this->meta.identifier[child->value_size()] = '\0';
        }
    }

    if (this->getMetaWidth() == 0) {
        this->meta.size.width = XML_VP_DEFAULT_WIDTH;
    }
    if (this->getMetaHeight() == 0) {
        this->meta.size.height = XML_VP_DEFAULT_HEIGHT;
    }

    return NIDIUM_XML_OK;
}

NativeNML::nidium_xml_ret_t NativeNML::loadAssets(rapidxml::xml_node<> &node)
{
    using namespace rapidxml;

    NativeAssets *assets = new NativeAssets(NativeNML_onAssetsItemRead,
        NativeNML_onAssetsReady, this);

    this->addAsset(assets);

    for (xml_node<> *child = node.first_node(); child != NULL;
        child = child->next_sibling())
    {
        xml_attribute<> *src = NULL;
        NativeAssets::Item *item = NULL;

        if ((src = child->first_attribute("src"))) {
            xml_attribute<> *id = child->first_attribute("id");
            item = new NativeAssets::Item(src->value(),
                NativeAssets::Item::ITEM_UNKNOWN, net, this->relativePath);

            /* Name could be automatically changed afterward */
            item->setName(src->value());

            assets->addToPendingList(item);
        } else {
            item = new NativeAssets::Item(NULL, NativeAssets::Item::ITEM_UNKNOWN, net);
            item->setName("inline"); /* TODO: NML name */
            assets->addToPendingList(item);
            item->setContent(child->value(), child->value_size(), true);
        }

        item->setTagName(child->name());
        
        if (!strncasecmp(child->name(), "script", 6)) {
            item->fileType = NativeAssets::Item::ITEM_SCRIPT;
        } else if (!strncasecmp(child->name(), "style", 6)) {
            item->fileType = NativeAssets::Item::ITEM_NSS;
        }
        //printf("Node : %s\n", child->name());
    }

    assets->endListUpdate(net);

    return NIDIUM_XML_OK;
}

NativeNML::nidium_xml_ret_t NativeNML::loadLayout(rapidxml::xml_node<> &node)
{
    m_Layout = node.document()->clone_node(&node);

    return NIDIUM_XML_OK;
}

bool NativeNML::loadData(char *data, size_t len, rapidxml::xml_document<> &doc)
{
    using namespace rapidxml;

    try {
        doc.parse<0>(data);
    } catch(rapidxml::parse_error &err) {
        printf("XML error : %s\n", err.what());

        return false;
    }

    xml_node<> *node = doc.first_node("application");
    if (node == NULL) {
        printf("XML : <application> node not found\n");
        return false;
    }

    for (xml_node<> *child = node->first_node(); child != NULL;
        child = child->next_sibling()) {
        for (int i = 0; nml_tags[i].str != NULL; i++) {
            if (!strncasecmp(nml_tags[i].str, child->name(),
                child->name_size())) {

                nidium_xml_ret_t ret;

                if ((ret = (this->*nml_tags[i].cb)(*child)) != NIDIUM_XML_OK) {
                    printf("XML : Nidium error (%d)\n", ret);
                    return false;
                }
            }
        }
    }

    return true;
}

/*
    <canvas>
        <next></next>   
    </canvas>
    <foo></foo>
*/
JSObject *NativeNML::buildLayoutTree(rapidxml::xml_node<> &node)
{
#define NODE_PROP(where, name, val) JS_DefineProperty(cx, where, name, \
    val, NULL, NULL, JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE)
#define NODE_STR(data, len) STRING_TO_JSVAL(JS_NewStringCopyN(cx, \
        (const char *)data, len))

    using namespace rapidxml;
    JSContext *cx = this->njs->cx;

    JS::RootedObject input(cx, JS_NewArrayObject(cx, 0, NULL));

    uint32_t idx = 0;
    for (xml_node<> *child = node.first_node(); child != NULL;
        child = child->next_sibling()) {

        /* obj */
        JS::RootedObject obj(cx, JS_NewObject(cx, NULL, NULL, NULL));

        /* obj.type */
        NODE_PROP(obj, "type", NODE_STR(child->name(), child->name_size()));

        /* obj.attributes */
        JS::RootedObject obj_attr(cx, JS_NewObject(cx, NULL, NULL, NULL));
        NODE_PROP(obj, "attributes", OBJECT_TO_JSVAL(obj_attr.get()));
        for (xml_attribute<> *attr = child->first_attribute(); attr != NULL;
            attr = attr->next_attribute()) {
            NODE_PROP(obj_attr, attr->name(), NODE_STR(attr->value(), attr->value_size()));
        }

        /* obj.children */
        JS::RootedObject obj_children(cx, this->buildLayoutTree(*child));
        NODE_PROP(obj, "children", OBJECT_TO_JSVAL(obj_children.get()));

        /* push to input array */
        jsval jobj = OBJECT_TO_JSVAL(obj);
        JS_SetElement(cx, input, idx++, &jobj);
    }
    return input;
#undef NODE_PROP
}

void NativeNML::onGetContent(const char *data, size_t len)
{
    rapidxml::xml_document<> doc;

    if (this->loadData((char *)data, len, doc)) {
        this->loaded(this->loaded_arg);
    }

    m_JSObjectLayout = this->buildLayoutTree(*m_Layout);
    JS_AddObjectRoot(this->njs->cx, &m_JSObjectLayout);

    /* Invalidate layout node since memory pool is free'd */
    m_Layout = NULL;
}
