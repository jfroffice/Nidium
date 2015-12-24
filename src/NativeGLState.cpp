#include "NativeGLState.h"

#include <stdlib.h>
#include <stddef.h>

#include "NativeCanvasContext.h"
#include "NativeTypes.h"
#include "NativeMacros.h"
#include "NativeOpenGLHeader.h"

NativeGLState::NativeGLState(NativeUIInterface *ui, bool withProgram, bool webgl) :
    m_Shared(true)
{
    memset(&this->m_GLObjects, 0, sizeof(this->m_GLObjects));
    memset(&this->m_GLObjects.uniforms, -1, sizeof(this->m_GLObjects.uniforms));

    m_GLContext = new NativeGLContext(ui, webgl ? NULL : ui->getGLContext(), webgl);

    if (!this->initGLBase(withProgram)) {
        NLOG("[OpenGL] Failed to init base GL");
    }
}

NativeGLState::NativeGLState(NativeContext *nctx) :
    m_Shared(true)
{
    NativeUIInterface *ui = nctx->getUI();

    memset(&this->m_GLObjects, 0, sizeof(this->m_GLObjects));
    memset(&this->m_GLObjects.uniforms, -1, sizeof(this->m_GLObjects.uniforms));

    m_GLContext = new NativeGLContext(ui, ui->getGLContext(), false);
}

void NativeGLState::CreateForContext(NativeContext *nctx)
{
    NativeUIInterface *ui;
    if ((ui = nctx->getUI()) == NULL || ui->NativeCtx->getGLState()) {
        NLOG("Failed to init the first NativeGLState");
        return;
    }

    NativeGLState *_this = new NativeGLState(nctx);

    nctx->setGLState(_this);
    _this->initGLBase(true);
}

void NativeGLState::destroy()
{
    if (!m_Shared) {
        delete this;
    }
}

void NativeGLState::setVertexDeformation(uint32_t vertex, float x, float y)
{
    NATIVE_GL_CALL_MAIN(BindBuffer(GL_ARRAY_BUFFER, m_GLObjects.vbo[0]));

    float mod[2] = {x, y};

    NATIVE_GL_CALL_MAIN(BufferSubData(GL_ARRAY_BUFFER,
        (sizeof(NativeVertex) * vertex) + offsetof(NativeVertex, Modifier),
        sizeof(((NativeVertex *)0)->Modifier),
        &mod));
}

bool NativeGLState::initGLBase(bool withProgram)
{
    //glctx->iface()->fExtensions.print();

    NATIVE_GL_CALL_MAIN(GenBuffers(2, m_GLObjects.vbo));
    NATIVE_GL_CALL_MAIN(GenVertexArrays(1, &m_GLObjects.vao));

    m_Resources.add(m_GLObjects.vbo[0], NativeGLResources::RBUFFER);
    m_Resources.add(m_GLObjects.vbo[1], NativeGLResources::RBUFFER);
    m_Resources.add(m_GLObjects.vao, NativeGLResources::RVERTEX_ARRAY);

    NativeVertices *vtx = m_GLObjects.vtx = NativeCanvasContext::buildVerticesStripe(4);

    NATIVE_GL_CALL_MAIN(BindVertexArray(m_GLObjects.vao));

    NATIVE_GL_CALL_MAIN(EnableVertexAttribArray(NativeCanvasContext::SH_ATTR_POSITION));
    NATIVE_GL_CALL_MAIN(EnableVertexAttribArray(NativeCanvasContext::SH_ATTR_TEXCOORD));
    NATIVE_GL_CALL_MAIN(EnableVertexAttribArray(NativeCanvasContext::SH_ATTR_MODIFIER));

    /* Upload the list of vertex */
    NATIVE_GL_CALL_MAIN(BindBuffer(GR_GL_ARRAY_BUFFER, m_GLObjects.vbo[0]));
    NATIVE_GL_CALL_MAIN(BufferData(GR_GL_ARRAY_BUFFER, sizeof(NativeVertex) * vtx->nvertices,
        vtx->vertices, GL_DYNAMIC_DRAW));

    /* Upload the indexes for triangle strip */
    NATIVE_GL_CALL_MAIN(BindBuffer(GR_GL_ELEMENT_ARRAY_BUFFER, m_GLObjects.vbo[1]));
    NATIVE_GL_CALL_MAIN(BufferData(GR_GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * vtx->nindices,
        vtx->indices, GL_STATIC_DRAW));

    NATIVE_GL_CALL_MAIN(VertexAttribPointer(NativeCanvasContext::SH_ATTR_POSITION, 3, GR_GL_FLOAT, GR_GL_FALSE,
                          sizeof(NativeVertex), 0));

    NATIVE_GL_CALL_MAIN(VertexAttribPointer(NativeCanvasContext::SH_ATTR_TEXCOORD, 2, GR_GL_FLOAT, GR_GL_FALSE,
                          sizeof(NativeVertex),
                          (GLvoid*) offsetof(NativeVertex, TexCoord)));

    NATIVE_GL_CALL_MAIN(VertexAttribPointer(NativeCanvasContext::SH_ATTR_MODIFIER, 2, GR_GL_FLOAT, GR_GL_FALSE,
                          sizeof(NativeVertex),
                          (GLvoid*) offsetof(NativeVertex, Modifier)));

    if (withProgram) {
        this->m_GLObjects.program = NativeCanvasContext::createPassThroughProgram(this->m_Resources);

        if (this->m_GLObjects.program == 0) {
            return false;
        }

        NATIVE_GL_CALL_RET_MAIN(GetUniformLocation(m_GLObjects.program, "u_projectionMatrix"),
            m_GLObjects.uniforms.u_projectionMatrix);

        NATIVE_GL_CALL_RET_MAIN(GetUniformLocation(m_GLObjects.program, "u_opacity"),
            m_GLObjects.uniforms.u_opacity);
    }

    NATIVE_GL_CALL_MAIN(BindVertexArray(0));

    return true;
}

void NativeGLState::setProgram(uint32_t program)
{
    this->m_GLObjects.program = program;

    NATIVE_GL_CALL_RET_MAIN(GetUniformLocation(m_GLObjects.program, "u_projectionMatrix"),
        m_GLObjects.uniforms.u_projectionMatrix);
    NATIVE_GL_CALL_RET_MAIN(GetUniformLocation(m_GLObjects.program, "u_opacity"),
        m_GLObjects.uniforms.u_opacity);
}

void NativeGLState::setActive()
{
    NATIVE_GL_CALL_MAIN(BindVertexArray(m_GLObjects.vao));
    NATIVE_GL_CALL_MAIN(ActiveTexture(GR_GL_TEXTURE0));
}

NativeGLState::~NativeGLState()
{
    free(m_GLObjects.vtx->indices);
    free(m_GLObjects.vtx->vertices);
    free(m_GLObjects.vtx);

    if (m_GLContext) {
        delete m_GLContext;
    }
}

