#include "NativeNML.h"
#include "NativeJS.h"

#include <string.h>
#include "NativeHash.h"
#include "NativeStream.h"

NativeNML::NativeNML(ape_global *net) :
    net(net), NFIO(NULL), relativePath(NULL), njs(NULL)
{
    assetsList.size = 0;
    assetsList.allocated = 4;

    assetsList.list = (NativeAssets **)malloc(sizeof(NativeAssets *) * assetsList.allocated);
}

NativeNML::~NativeNML()
{
    if (relativePath) {
        free(relativePath);
    }
    if (NFIO) {
        delete NFIO;
    }
    for (int i = 0; i < assetsList.size; i++) {
        delete assetsList.list[i];
    }
    free(assetsList.list);
}

void NativeNML::loadFile(const char *file)
{
    this->relativePath = NativeStream::resolvePath(file, NativeStream::STREAM_RESOLVE_PATH);

    NFIO = new NativeFileIO(file, this, this->net);
    NFIO->open("r");
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
            
            break;
        default:
            njs->assetReady(tag);
            break;
    }
}

static void NativeNML_onAssetsItemRead(NativeAssets::Item *item, void *arg)
{
    class NativeNML *nml = (class NativeNML *)arg;

    nml->onAssetsItemReady(item);
}

void NativeNML::addAsset(NativeAssets *asset)
{
    if (assetsList.size == assetsList.allocated) {
        assetsList.allocated *= 2;
        assetsList.list = (NativeAssets **)realloc(assetsList.list, sizeof(NativeAssets *) * assetsList.allocated);
    }

    assetsList.list[assetsList.size] = asset;

    assetsList.size++;
}

void NativeNML::loadAssets(rapidxml::xml_node<> &node)
{
    using namespace rapidxml;

    NativeAssets *assets = new NativeAssets(NativeNML_onAssetsItemRead, this);

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

            item->setName(id != NULL ? id->value() : src->value());

            assets->addToPendingList(item);
        } else {
            item = new NativeAssets::Item(NULL, NativeAssets::Item::ITEM_UNKNOWN, net);
            item->setName("inline");
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
}


void NativeNML::loadData(char *data, size_t len)
{
    using namespace rapidxml;

    xml_document<> doc;

    try {
        doc.parse<0>(data);
    } catch(rapidxml::parse_error &err) {
        printf("XML error : %s\n", err.what());

        return;
    }

    xml_node<> *node = doc.first_node("application");
    if (node == NULL) {
        printf("XML : <application> node not found\n");
        return;
    }

    for (xml_node<> *child = node->first_node(); child != NULL;
        child = child->next_sibling())
    {
        for (int i = 0; nml_tags[i].str != NULL; i++) {
            if (!strncasecmp(nml_tags[i].str, child->name(),
                child->name_size())) {

                (this->*nml_tags[i].cb)(*child);
            }
        }
    }

}

void NativeNML::onNFIOOpen(NativeFileIO *NFIO)
{
    NFIO->read(NFIO->filesize);
}

void NativeNML::onNFIORead(NativeFileIO *NFIO, unsigned char *data, size_t len)
{
    this->loadData((char *)data, len);
}
