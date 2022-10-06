#include <limits>

#include "guiwrapper.h"

using namespace Vipster;

GuiWrapper::GuiWrapper(const Settings &s)
    : settings{s}
{
    selection.color = s.selCol.val;
}

void GuiWrapper::initGL()
{
#ifndef WEBVIPSTER
    initializeOpenGLFunctions();
#endif
    // init ViewUBO
    glGenBuffers(1, &view_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, view_ubo);
    glBufferData(GL_UNIFORM_BUFFER, 2*sizeof(GUI::Mat_16f), nullptr, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, view_ubo);

    // init view matrices
    oMat = guiMatMkOrtho(-15, 15, -10, 10, -100, 1000);
    pMat = guiMatMkPerspective(60.0, 1.5, 0.001f, 1000);
    rMat = {{1,0,0,0,
             0,1,0,0,
             0,0,1,0,
             0,0,0,1}};
    vMat = guiMatMkLookAt({{0,0,10}}, {{0,0,0}}, {{0,1,0}});
    pMatChanged = rMatChanged = vMatChanged = true;
    drawPerspective = settings.perspective.val;
}

void GuiWrapper::drawPre(void *context)
{
    // reset OGL state
#ifndef __EMSCRIPTEN__
    if(settings.antialias.val){
        glEnable(GL_MULTISAMPLE);
    }else{
        glDisable(GL_MULTISAMPLE);
    }
#endif
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    const auto &bgCol = settings.bgCol.val;
    glClearColor(bgCol[0]/255., bgCol[1]/255., bgCol[2]/255., bgCol[3]/255.);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // synchronize data
    mainStep.syncToGPU(context);
    selection.syncToGPU(context);
    if(stepExtras){
        for(const auto& i: *stepExtras){
            if(auto j=i.lock()){
                j->syncToGPU(context);
            }
        }
    }
    if(vpExtras){
        for(const auto& i: *vpExtras){
            if(auto j=i.lock()){
                j->syncToGPU(context);
            }
        }
    }
}

void GuiWrapper::drawImpl(const Vec &pos, void *context)
{
    Vec off = pos - curStep->getCenter(AtomFmt::Bohr, settings.rotCom.val);
    Mat cv = {{{{1,0,0}}, {{0,1,0}}, {{0,0,1}}}};
    if(curStep->hasCell()){
        cv = curStep->getCellVec() * curStep->getCellDim(AtomFmt::Bohr);
        off -= (mult[0]-1)*cv[0]/2.;
        off -= (mult[1]-1)*cv[1]/2.;
        off -= (mult[2]-1)*cv[2]/2.;
    }
    auto m = curStep->hasCell() ? mult : GUI::PBCVec{{1,1,1}};
    mainStep.draw(off, m, cv, settings.showCell.val, context);
    selection.draw(off, m, cv, settings.showCell.val, context);
    if(stepExtras){
        for(const auto& i: *stepExtras){
            if(auto j=i.lock()){
                j->draw(off, m, cv, settings.showCell.val, context);
            }
        }
    }
    if(vpExtras){
        for(const auto& i: *vpExtras){
            if(auto j=i.lock()){
                j->draw(off, m, cv, settings.showCell.val, context);
            }
        }
    }
}

void GuiWrapper::draw(void *context)
{
    drawPre(context);
    if(drawPerspective != settings.perspective.val){
        drawPerspective = settings.perspective.val;
        pMatChanged = true;
    }
    if(rMatChanged||pMatChanged||vMatChanged){
        updateViewUBO();
    }
    drawImpl(Vec{0,0,0}, context);
}

void GuiWrapper::drawVR(const float *leftProj, const float *leftView,
                        const float *rightProj, const float* rightView,
                        const Vec& pos,
                        unsigned long width, unsigned long height)
{
    drawPre();
    // render left eye
    updateViewUBOVR(leftProj, leftView);
    glViewport(0,0,width/2,height);
    drawImpl(pos);
    // render right eye
    updateViewUBOVR(rightProj, rightView);
    glViewport(width/2,0,width/2,height);
    drawImpl(pos);
}

void GuiWrapper::drawSel(void *context)
{
    mainStep.syncToGPU(context);
#ifndef __EMSCRIPTEN__
    if(settings.antialias.val){
        glDisable(GL_MULTISAMPLE);
    }
#endif
    Vec off = -curStep->getCenter(AtomFmt::Bohr, settings.rotCom.val);
    mainStep.drawSel(off, mult, context);
#ifndef __EMSCRIPTEN__
    if(settings.antialias.val){
        glEnable(GL_MULTISAMPLE);
    }
#endif
}

void GuiWrapper::setMainStep(Step* step)
{
    curStep = step;
    updateMainStep();
}

void GuiWrapper::setMainSel(Step::selection* sel)
{
    curSel = sel;
    updateMainSelection();
}

void GuiWrapper::updateMainStep()
{
    mainStep.update(curStep, settings.atRadVdW.val,
                    settings.atRadFac.val, settings.bondRad.val);
}

void GuiWrapper::updateMainSelection()
{
    selection.update(curSel, settings.atRadVdW.val, settings.atRadFac.val);
}

void GuiWrapper::updateViewUBOVR(const float* proj, const float* view)
{
    glBindBuffer(GL_UNIFORM_BUFFER, view_ubo);
    auto pTemp = GUI::Mat_16f{proj[0],  proj[4],  proj[8],  proj[12],
                          proj[1],  proj[5],  proj[9],  proj[13],
                          proj[2],  proj[6],  proj[10], proj[14],
                          proj[3], proj[7], proj[11], proj[15]};
    auto vTemp = GUI::Mat_16f{view[0],  view[4],  view[8],  view[12],
                          view[1],  view[5],  view[9],  view[13],
                          view[2],  view[6],  view[10], view[14],
                          view[3], view[7], view[11], view[15]};
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GUI::Mat_16f),
                    (pTemp*vTemp).data());
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(GUI::Mat_16f), sizeof(GUI::Mat_16f),
                    GUI::Mat_16f{1,0,0,0,
                             0,1,0,0,
                             0,0,1,0,
                             0,0,0,1}.data());
    // make sure mirroring resets matrices
    rMatChanged = true;
}

void GuiWrapper::updateViewUBO(void)
{
    if(rMatChanged){
        glBindBuffer(GL_UNIFORM_BUFFER, view_ubo);
        if(settings.perspective.val){
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GUI::Mat_16f),
                            (pMat*vMat*rMat).data());
        }else{
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GUI::Mat_16f),
                            (oMat*vMat*rMat).data());
        }
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(GUI::Mat_16f), sizeof(GUI::Mat_16f), rMat.data());
        rMatChanged = vMatChanged = pMatChanged = false;
    }else if (pMatChanged || vMatChanged){
        glBindBuffer(GL_UNIFORM_BUFFER, view_ubo);
        if(settings.perspective.val){
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GUI::Mat_16f),
                            (pMat*vMat*rMat).data());
        }else{
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GUI::Mat_16f),
                            (oMat*vMat*rMat).data());
        }
        vMatChanged = pMatChanged = false;
    }
}

void GuiWrapper::resizeViewMat(long w, long h)
{
    h==0?h=1:0;
    glViewport(0,0,w,h);
    float aspect = float(w)/h;
    oMat = guiMatMkOrtho(-10*aspect, 10*aspect, -10, 10, -100, 1000);
    pMat = guiMatMkPerspective(60, aspect, 0.001f, 1000);
    pMatChanged = true;
}

void GuiWrapper::zoomViewMat(float i)
{
    guiMatScale(vMat, i);
    vMatChanged = true;
}

void GuiWrapper::rotateViewMat(float x, float y, float z)
{
    guiMatRot(rMat, x, 0, 1, 0);
    guiMatRot(rMat, y, 1, 0, 0);
    guiMatRot(rMat, z, 0, 0, 1);
    rMatChanged = true;
}

void GuiWrapper::translateViewMat(float x, float y, float z)
{
    guiMatTranslate(vMat, x/10.f, y/10.f, z/10.f);
    vMatChanged = true;
}

void GuiWrapper::alignViewMat(alignDir d)
{
    switch (d) {
    case alignDir::x:
        rMat = {{ 0, 1, 0, 0,
                  0, 0, 1, 0,
                  1, 0, 0, 0,
                  0, 0, 0, 1}};
        break;
    case alignDir::mx:
        rMat = {{ 0,-1, 0, 0,
                  0, 0, 1, 0,
                 -1, 0, 0, 0,
                  0, 0, 0, 1}};
        break;
    case alignDir::y:
        rMat = {{-1, 0, 0, 0,
                  0, 0, 1, 0,
                  0, 1, 0, 0,
                  0, 0, 0, 1}};
        break;
    case alignDir::my:
        rMat = {{ 1, 0, 0, 0,
                  0, 0, 1, 0,
                  0,-1, 0, 0,
                  0, 0, 0, 1}};
        break;
    case alignDir::z:
        rMat = {{ 1, 0, 0, 0,
                  0, 1, 0, 0,
                  0, 0, 1, 0,
                  0, 0, 0, 1}};
        break;
    case alignDir::mz:
        rMat = {{-1, 0, 0, 0,
                  0, 1, 0, 0,
                  0, 0,-1, 0,
                  0, 0, 0, 1}};
        break;
    }
    rMatChanged = true;
}

Mat GuiWrapper::getAxes()
{
    Mat tmp;
    tmp[0] =  Vec{rMat[0], rMat[1], rMat[2]};
    tmp[1] = -Vec{rMat[4], rMat[5], rMat[6]};
    tmp[2] =  Vec{rMat[8], rMat[9], rMat[10]};
    return tmp;
}
