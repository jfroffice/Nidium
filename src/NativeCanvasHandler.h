#ifndef nativecanvashandler_h__
#define nativecanvashandler_h__

#include <stdint.h>

class NativeSkia;
class NativeCanvas2DContext;

/*
    Handle a canvas layer.
    Agnostic to any renderer.
    A NativeCanvasContext must be attached to it

    TODO:
        * NativeCanvasContext interface instead of NativeCanvas2DContext;
*/

struct NativeRect
{
    double fLeft, fTop, fBottom, fRight;
    bool isEmpty() const { return fLeft >= fRight || fTop >= fBottom; }
    bool intersect(double left, double top, double right, double bottom) {
        if (left < right && top < bottom && !this->isEmpty() &&
            fLeft < right && left < fRight && fTop < bottom && top < fBottom)
        {
            if (fLeft < left) fLeft = left;
            if (fTop < top) fTop = top;
            if (fRight > right) fRight = right;
            if (fBottom > bottom) fBottom = bottom;
            return true;
        }
        return false;
    }
    bool contains(double x, double y) const {
        return !this->isEmpty() &&
               fLeft <= x && x < fRight &&
               fTop <= y && y < fBottom;
    }
};

class NativeCanvasHandler
{
    public:
        friend class NativeSkia;

        enum Position {
            POSITION_FRONT,
            POSITION_BACK
        };
        enum COORD_POSITION {
            COORD_RELATIVE,
            COORD_ABSOLUTE,
            COORD_FIXED
        };

        enum Visibility {
            CANVAS_VISIBILITY_VISIBLE,
            CANVAS_VISIBILITY_HIDDEN
        };

        NativeCanvas2DContext *context;
        class JSObject *jsobj;
        struct JSContext *jscx;

        int width, height;
        /*
            left and top are relative to parent
            a_left and a_top are relative to the root layer
        */
        double left, top, a_left, a_top;

        struct {
            double top;
            double bottom;
            double left;
            double right;
            int global;
        } padding;

        struct {
            double x;
            double y;
        } translate_s;

        struct {
            int width;
            int height;
            int scrollTop;
            int scrollLeft;
        } content;

        struct {
            int x, y, xrel, yrel;
        } mousePosition;

        double opacity;
        bool overflow;
        
        NativeCanvasHandler(int width, int height);
        ~NativeCanvasHandler();

        void unrootHierarchy();
        void setWidth(int width);
        void setHeight(int height);
        void setSize(int width, int height);
        void setPadding(int padding);
        void setPositioning(NativeCanvasHandler::COORD_POSITION mode);
        void setScrollTop(int value);
        void setScrollLeft(int value);
        void computeAbsolutePosition();
        void computeContentSize(int *cWidth, int *cHeight);
        void translate(double x, double y);
        bool isOutOfBound();
        NativeRect getViewport();
        NativeRect getVisibleRect();

        void bringToFront();
        void sendToBack();
        void addChild(NativeCanvasHandler *insert,
            NativeCanvasHandler::Position position = POSITION_FRONT);

        int getContentWidth();
        int getContentHeight();
        void setHidden(bool val);
        bool isDisplayed() const;
        bool isHidden() const;
        bool hasAFixedAncestor() const;
        void setOpacity(double val);
        void removeFromParent();
        void getChildren(NativeCanvasHandler **out) const;
        NativeCanvasHandler *getParent() const { return this->parent; }
        NativeCanvasHandler *getFirstChild() const { return this->children; }
        NativeCanvasHandler *getLastChild() const { return this->last; }
        NativeCanvasHandler *getNextSibling() const { return this->next; }
        NativeCanvasHandler *getPrevSibling() const { return this->prev; }
        int32_t countChildren() const;
        bool containsPoint(double x, double y) const;
        void layerize(NativeCanvasHandler *layer, double pleft,
            double ptop, double aopacity, NativeRect *clip);
        NativeCanvasHandler *parent;
        NativeCanvasHandler *children;
        NativeCanvasHandler *next;
        NativeCanvasHandler *prev;
        NativeCanvasHandler *last;
    private:

        int32_t nchildren;

        COORD_POSITION coordPosition;
        Visibility visibility;
};

#endif
