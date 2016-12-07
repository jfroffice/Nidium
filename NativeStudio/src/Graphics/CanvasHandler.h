/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#ifndef graphics_canvashandler_h__
#define graphics_canvashandler_h__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <jsapi.h>

#include <Core/Events.h>

/*
    - Handle a canvas layer.
    - Agnostic to any renderer.
    - All size are in logical pixels (device ratio is handled by CanvasContext)
*/

namespace Nidium {
namespace Interface {
    class UIInterface;
}
namespace Binding {
    class JSCanvas;
}
namespace Frontend {
    class Context;
    class InputEvent;
}
namespace Graphics {

class CanvasHandler;
class SkiaContext;
class CanvasContext;

// {{{ Rect
struct Rect
{
    double m_fLeft, m_fTop, m_fBottom, m_fRight;
    bool isEmpty() const { return m_fLeft >= m_fRight || m_fTop >= m_fBottom; }
    bool intersect(double left, double top, double right, double bottom) {
        if (left < right && top < bottom && !this->isEmpty() &&
            m_fLeft < right && left < m_fRight && m_fTop < bottom && top < m_fBottom)
        {
            if (m_fLeft < left) m_fLeft = left;
            if (m_fTop < top) m_fTop = top;
            if (m_fRight > right) m_fRight = right;
            if (m_fBottom > bottom) m_fBottom = bottom;
            return true;
        }
        return false;
    }

    bool checkIntersect(double left, double top, double right, double bottom) const {
        if (left < right && top < bottom && !this->isEmpty() &&
            m_fLeft < right && left < m_fRight && m_fTop < bottom && top < m_fBottom)
        {
            return true;
        }
        return false;
    }

    Rect scaled(float scale) const {
        Rect r = {
            m_fLeft*scale,
            m_fTop*scale,
            m_fBottom*scale,
            m_fRight*scale
        };

        return r;
    }
    bool contains(double x, double y) const {
        return !this->isEmpty() &&
               m_fLeft <= x && x < m_fRight &&
               m_fTop <= y && y < m_fBottom;
    }
};
// }}}

// {{{ LayerSiblingContext
struct LayerSiblingContext {
    double m_MaxLineHeight;
    double m_MaxLineHeightPreviousLine;

    LayerSiblingContext() :
        m_MaxLineHeight(0.0), m_MaxLineHeightPreviousLine(0.0) {}
};
// }}}

// {{{ LayerizeContext
struct LayerizeContext {
    CanvasHandler *m_Layer;
    double m_pLeft;
    double m_pTop;
    double m_aOpacity;
    double m_aZoom;
    Rect *m_Clip;

    struct LayerSiblingContext *m_SiblingCtx;

    void reset() {
        m_Layer = NULL;
        m_pLeft = 0.;
        m_pTop = 0.;
        m_aOpacity = 1.0;
        m_aZoom = 1.0;
        m_Clip = NULL;
        m_SiblingCtx = NULL;
    }
};
// }}}

// {{{ CanvasHandler
class CanvasHandler : public Core::Events
{
    public:
        friend class SkiaContext;
        friend class Nidium::Frontend::Context;
        friend class Binding::JSCanvas;

        static const uint8_t EventID = 1;
        static int m_LastIdx;

        enum COORD_MODE {
            kLeft_Coord   = 1 << 0,
            kRight_Coord  = 1 << 1,
            kTop_Coord    = 1 << 2,
            kBottom_Coord = 1 << 3
        };

        enum Flags {
            kDrag_Flag = 1 << 0
        };

        enum EventsChangedProperty {
            kContentHeight_Changed,
            kContentWidth_Changed
        };

        enum Events {
            RESIZE_EVENT = 1,
            LOADED_EVENT,
            CHANGE_EVENT,
            MOUSE_EVENT,
            DRAG_EVENT
        };

        enum Position {
            POSITION_FRONT,
            POSITION_BACK
        };

        enum COORD_POSITION {
            COORD_RELATIVE,
            COORD_ABSOLUTE,
            COORD_FIXED,
            COORD_INLINE,
            COORD_INLINEBREAK
        };

        enum FLOW_MODE {
            kFlowDoesntInteract = 0,
            kFlowInlinePreviousSibling = 1 << 0,
            kFlowBreakPreviousSibling = 1 << 1,
            kFlowBreakAndInlinePreviousSibling = (kFlowInlinePreviousSibling | kFlowBreakPreviousSibling)
        };

        enum Visibility {
            CANVAS_VISIBILITY_VISIBLE,
            CANVAS_VISIBILITY_HIDDEN
        };

        CanvasContext *m_Context;
        JS::Heap<JSObject *> m_JsObj;
        JSContext *m_JsCx;

        int m_Width, m_Height, m_MinWidth, m_MinHeight, m_MaxWidth, m_MaxHeight;
        /*
            left and top are relative to parent
            a_left and a_top are relative to the root layer
        */
        double m_Left, m_Top, m_aLeft, m_aTop, m_Right, m_Bottom;

        struct {
            double top;
            double bottom;
            double left;
            double right;
            int global;
        } m_Padding;

        struct {
            double top;
            double right;
            double bottom;
            double left;
        } m_Margin;

        struct {
            double x;
            double y;
        } m_Translate_s;

        struct {
            int width;
            int height;
            int scrollTop;
            int scrollLeft;
            int _width, _height;
        } m_Content;

        struct {
            int x, y, xrel, yrel;
            bool consumed;
        } m_MousePosition;

        bool m_Overflow;

        CanvasContext *getContext() const {
            return m_Context;
        }

        double getOpacity() const {
            return m_Opacity;
        }

        double getZoom() const {
            return m_Zoom;
        }

        double getLeft(bool absolute = false) const {
            if (absolute) return m_aLeft;

            if (!(m_CoordMode & kLeft_Coord) && m_Parent) {
                return m_Parent->getWidth() - (m_Width + m_Right);
            }

            return m_Left;
        }
        double getTop(bool absolute = false) const {
            if (absolute) return m_aTop;

            if (!(m_CoordMode & kTop_Coord) && m_Parent) {
                return m_Parent->getHeight() - (m_Height + m_Bottom);
            }

            return m_Top;
        }

        double getTopScrolled() const {
            double top = getTop();
            if (m_CoordPosition == COORD_RELATIVE && m_Parent != NULL) {
                top -= m_Parent->m_Content.scrollTop;
            }

            return top;
        }

        double getLeftScrolled() const {
            double left = getLeft();
            if (m_CoordPosition == COORD_RELATIVE && m_Parent != NULL) {
                left -= m_Parent->m_Content.scrollLeft;
            }
            return left;
        }

        double getRight() const {
            if (hasStaticRight() || !m_Parent) {
                return m_Right;
            }

            return m_Parent->getWidth() - (getLeftScrolled() + getWidth());
        }

        double getBottom() const {
            if (hasStaticBottom() || !m_Parent) {
                return m_Bottom;
            }

            return m_Parent->getHeight() - (getTopScrolled() + getHeight());
        }

        /*
            Get the width in logical pixels
        */
        double getWidth() const {
            if (hasFixedWidth() || m_FluidWidth) {
                return m_Width;
            }
            if (m_Parent == NULL) return 0.;

            double pwidth = m_Parent->getWidth();

            if (pwidth == 0) return 0.;

            return nidium_max(pwidth - this->getLeft() - this->getRight(), 1);
        }

        /*
            Get the height in logical pixels
        */
        double getHeight() const {
            if (hasFixedHeight() || m_FluidHeight) {
                return m_Height;
            }

            if (m_Parent == NULL) return 0.;

            double pheight = m_Parent->getHeight();

            if (pheight == 0) return 0.;

            return nidium_max(pheight - this->getTop() - this->getBottom(), 1);
        }

        int getMinWidth() const {
            return m_MinWidth;
        }

        int getMaxWidth() const {
            return m_MaxWidth;
        }

        int getMinHeight() const {
            return m_MinHeight;
        }

        int getMaxHeight() const {
            return m_MaxHeight;
        }

        Frontend::Context *getNativeContext() const {
            return m_NativeContext;
        }

        bool hasFixedWidth() const {
            return !((m_CoordMode & (kLeft_Coord | kRight_Coord))
                    == (kLeft_Coord|kRight_Coord) || m_FluidWidth);
        }

        bool hasFixedHeight() const {
            return !((m_CoordMode & (kTop_Coord | kBottom_Coord))
                    == (kTop_Coord|kBottom_Coord) || m_FluidHeight);
        }

        bool hasStaticLeft() const {
            return m_CoordMode & kLeft_Coord;
        }

        bool hasStaticRight() const {
            return m_CoordMode & kRight_Coord;
        }

        bool hasStaticTop() const {
            return m_CoordMode & kTop_Coord;
        }

        bool hasStaticBottom() const {
            return m_CoordMode & kBottom_Coord;
        }

        void unsetLeft() {
            m_CoordMode &= ~kLeft_Coord;
        }

        void unsetRight() {
            m_CoordMode &= ~kRight_Coord;
        }

        void unsetTop() {
            m_CoordMode &= ~kTop_Coord;
        }

        void unsetBottom() {
            m_CoordMode &= ~kBottom_Coord;
        }

        void setMargin(double top, double right, double bottom, double left)
        {
            m_Margin.top = top;
            m_Margin.right = right;
            m_Margin.bottom = bottom;
            m_Margin.left = left;
        }

        void setLeft(double val) {
            if (m_FlowMode & kFlowInlinePreviousSibling) {
                return;
            }
            m_CoordMode |= kLeft_Coord;
            m_Left = val;
            if (!hasFixedWidth()) {
                setSize(this->getWidth(), m_Height);
            }
        }
        void setRight(double val) {
            m_CoordMode |= kRight_Coord;
            m_Right = val;
            if (!hasFixedWidth()) {
                setSize(this->getWidth(), m_Height);
            }
        }

        void setTop(double val) {
            if (m_FlowMode & kFlowInlinePreviousSibling) {
                return;
            }
            m_CoordMode |= kTop_Coord;
            m_Top = val;
            if (!hasFixedHeight()) {
                setSize(m_Width, this->getHeight());
            }
        }

        void setBottom(double val) {
            m_CoordMode |= kBottom_Coord;
            m_Bottom = val;

            if (!hasFixedHeight()) {
                setSize(m_Width, this->getHeight());
            }
        }

        void setScale(double x, double y);

        void setId(const char *str);

        double getScaleX() const {
            return m_ScaleX;
        }

        double getScaleY() const {
            return m_ScaleY;
        }

        uint64_t getIdentifier(char **str = NULL) {
            if (str != NULL) {
                *str = m_Identifier.str;
            }

            return m_Identifier.idx;
        }

        void setAllowNegativeScroll(bool val) {
            m_AllowNegativeScroll = val;
        }

        bool getAllowNegativeScroll() const {
            return m_AllowNegativeScroll;
        }

        COORD_POSITION getPositioning() const {
            return m_CoordPosition;
        }

        unsigned int getFlowMode() const {
            return m_FlowMode;
        }

        bool isHeightFluid() const {
            return m_FluidHeight;
        }

        bool isWidthFluid() const {
            return m_FluidWidth;
        }

        CanvasHandler(int width, int height,
            Frontend::Context *NativeCtx, bool lazyLoad = false);

        virtual ~CanvasHandler();

        void unrootHierarchy();

        void setContext(CanvasContext *context);

        bool setWidth(int width, bool force = false);
        bool setHeight(int height, bool force = false);

        bool setMinWidth(int width);
        bool setMinHeight(int height);

        bool setMaxWidth(int width);
        bool setMaxHeight(int height);

        bool setFluidHeight(bool val);
        bool setFluidWidth(bool val);

        void updateChildrenSize(bool width, bool height);
        void setSize(int width, int height, bool redraw = true);
        void setPadding(int padding);
        void setPositioning(CanvasHandler::COORD_POSITION mode);
        void setScrollTop(int value);
        void setScrollLeft(int value);
        void computeAbsolutePosition();
        void computeContentSize(int *cWidth, int *cHeight, bool inner = false);
        void translate(double x, double y);
        bool isOutOfBound();
        Rect getViewport();
        Rect getVisibleRect();

        void bringToFront();
        void sendToBack();
        void addChild(CanvasHandler *insert,
            CanvasHandler::Position position = POSITION_FRONT);

        void insertBefore(CanvasHandler *insert, CanvasHandler *ref);
        void insertAfter(CanvasHandler *insert, CanvasHandler *ref);

        int getContentWidth(bool inner = false);
        int getContentHeight(bool inner = false);
        void setHidden(bool val);
        bool isDisplayed() const;
        bool isHidden() const;
        bool hasAFixedAncestor() const;
        void setOpacity(double val);
        void setZoom(double val);
        void removeFromParent(bool willBeAdopted = false);
        void getChildren(CanvasHandler **out) const;

        bool checkLoaded();

        void setCursor(int cursor);
        int getCursor();

        CanvasHandler *getParent() const { return m_Parent; }
        CanvasHandler *getFirstChild() const { return m_Children; }
        CanvasHandler *getLastChild() const { return m_Last; }
        CanvasHandler *getNextSibling() const { return m_Next; }
        CanvasHandler *getPrevSibling() const { return m_Prev; }
        int32_t countChildren() const;
        bool containsPoint(double x, double y) const;
        void layerize(LayerizeContext &layerContext, bool draw);

        CanvasHandler *m_Parent;
        CanvasHandler *m_Children;

        CanvasHandler *m_Next;
        CanvasHandler *m_Prev;
        CanvasHandler *m_Last;

        static void _JobResize(void *arg);
        bool _handleEvent(Frontend::InputEvent *ev);

        uint32_t m_Flags;

    protected:
        CanvasHandler *getPrevInlineSibling() const {
            CanvasHandler *prev;
            for (prev = m_Prev; prev != NULL; prev = prev->m_Prev) {
                if (prev->m_FlowMode & kFlowInlinePreviousSibling) {
                    return prev;
                }
            }

            return NULL;
        }

        void propertyChanged(EventsChangedProperty property);
    private:
        void execPending();
        void deviceSetSize(int width, int height);
        void onMouseEvent(Frontend::InputEvent *ev);
        void onDrag(Frontend::InputEvent *ev, CanvasHandler *target, bool end = false);
        void onDrop(Frontend::InputEvent *ev, CanvasHandler *droped);

        int32_t m_nChildren;
        void dispatchMouseEvents(LayerizeContext &layerContext);
        COORD_POSITION m_CoordPosition;
        Visibility m_Visibility;
        unsigned m_FlowMode;
        unsigned m_CoordMode : 16;
        double m_Opacity;
        double m_Zoom;

        double m_ScaleX, m_ScaleY;
        bool m_AllowNegativeScroll;
        bool m_FluidWidth, m_FluidHeight;

        Frontend::Context *m_NativeContext;

        struct {
            uint64_t idx;
            char *str;
        } m_Identifier;

        void recursiveScale(double x, double y, double oldX, double oldY);
        void setPendingFlags(int flags, bool append = true);

        enum PENDING_JOBS {
            kPendingResizeWidth = 1 << 0,
            kPendingResizeHeight = 1 << 1,
        };

        int m_Pending;
        bool m_Loaded;
        int m_Cursor;
};
// }}}

} // namespace Graphics
} // namespace Nidium

#endif

