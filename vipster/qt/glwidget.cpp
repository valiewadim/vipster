#include "glwidget.h"
#include <QKeyEvent>
#include <QOpenGLFramebufferObject>
#include <QApplication>
#include <QPainter>

using namespace Vipster;

GLWidget::GLWidget(QWidget *parent):
    QOpenGLWidget(parent),
    // WARNING: this has to fail somewhere down the line, at the latest when multiple widgets are used
    GuiWrapper{qobject_cast<MainWindow*>(qApp->topLevelWidgets()[0])->settings}
{
    setTextureFormat(GL_RGBA16);
    for(auto *w: qApp->topLevelWidgets()){
        if(auto *t = qobject_cast<MainWindow*>(w)){
            master = t;
            return;
        }
    }
    throw Error("Could not determine MainWindow-instance.");
}

GLWidget::~GLWidget()
{
    makeCurrent();

    doneCurrent();
}

void GLWidget::triggerUpdate(GUI::change_t change){
    updateTriggered = true;
    master->updateWidgets(change);
}

void GLWidget::updateWidget(GUI::change_t change)
{
    if((change & GUI::stepChanged) == GUI::stepChanged ){
        setMainStep(master->curStep);
        setMainSel(master->curSel);
    }else{
        if(change & GUI::Change::settings){
            selection.update(master->settings.selCol.val);
        }
        if(change & (GUI::Change::atoms | GUI::Change::cell | GUI::Change::fmt | GUI::Change::settings)) {
            updateMainStep();
            updateMainSelection();
        }else if(change & GUI::Change::selection){
            updateMainSelection();
        }
    }
    update();
}

void GLWidget::initializeGL()
{
    makeCurrent();
    initGL("# version 330\n", ":/shaders");
}

void GLWidget::paintGL()
{
    QPainter painter{this};
    painter.beginNativePainting();
    draw();
    if(rectPos != mousePos){
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        painter.endNativePainting();
        QPen pen{};
        pen.setWidth(2);
        pen.setStyle(Qt::SolidLine);
        pen.setColor(Qt::black);
        painter.setPen(pen);
        QBrush brush{};
        brush.setColor(QColor{180,180,180,40});
        brush.setStyle(Qt::SolidPattern);
        painter.setBrush(brush);
        painter.drawRect(QRect{mousePos, rectPos});
    }else{
        painter.endNativePainting();
    }
}

void GLWidget::resizeGL(int w, int h)
{
    resizeViewMat(w, h);
}

void GLWidget::setMode(int mode, bool t)
{
    if(!t) {
        return;
    }
    mouseMode = static_cast<MouseMode>(mode);
    switch(mouseMode){
    case MouseMode::Camera:
        setCursor(Qt::ArrowCursor);
        break;
    case MouseMode::Modify:
        setCursor(Qt::OpenHandCursor);
        break;
    case MouseMode::Select:
        setCursor(Qt::CrossCursor);
        break;
    case MouseMode::Bond:
        setCursor(Qt::CrossCursor);
        break;
    }
}

void GLWidget::setMult(GUI::PBCVec m)
{
    mult = m;
    update();
}

void GLWidget::setCamera(int i)
{
    alignViewMat(static_cast<alignDir>((-i)-2));
    update();
}

void GLWidget::keyPressEvent(QKeyEvent *e)
{
    switch(e->key()){
    case Qt::Key_Down:
        rotateViewMat(0, 10, 0);
        break;
    case Qt::Key_Up:
        rotateViewMat(0, -10, 0);
        break;
    case Qt::Key_Left:
        rotateViewMat(-10, 0, 0);
        break;
    case Qt::Key_Right:
        rotateViewMat(10, 0, 0);
        break;
    default:
        return;
    }
    e->accept();
    update();
}

std::map<size_t, std::vector<SizeVec> > GLWidget::pickAtoms(QPoint from, QPoint to)
{
    // return indices of atoms enclosed by rectangle
    // defined by `from` and `to`
    std::map<size_t, std::vector<SizeVec>> idx;
    makeCurrent();
    drawSel();
    QOpenGLFramebufferObjectFormat format;
    format.setSamples(0);
    format.setInternalTextureFormat(GL_RGBA16);
    // TODO: benchmark smaller framebuffer
//    QOpenGLFramebufferObject fbo{QSize{w, h}, format};
    QOpenGLFramebufferObject fbo{size(), format};
    glBindFramebuffer(GL_READ_FRAMEBUFFER, defaultFramebufferObject());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo.handle());
    glBlitFramebuffer(0, 0, width(), height(),
                      0, 0, fbo.width(), fbo.height(),
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    fbo.bind();
    auto x = std::min(from.x(), to.x());
    auto y = std::min(height() - 1 - from.y(),
                      height() - 1 - to.y());
    auto w = std::max(1, std::abs(from.x() - to.x()));
    auto h = std::max(1, std::abs(from.y() - to.y()));
    std::vector<GLuint64> data(static_cast<size_t>(w*h));
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 2);
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_SHORT, data.data());
    fbo.release();
    std::set<GLuint64> set{data.begin(), data.end()};
    //
    for(const auto& p: set){
        auto i = static_cast<GLuint>(p&0xFFFFFFFF);
        auto box = static_cast<GLuint>((p&0xFFFFFFFF00000000)>>32);
        if(box){
            //untangle pbc-id
            box -= 1;
            auto z = box / (mult[0]*mult[1]);
            auto yrem = box % (mult[0]*mult[1]);
            auto y = yrem / mult[0];
            auto x = yrem % mult[0];
            // untangle atom-id
            idx[i].push_back({x,y,z});
        }
    }
    return idx;
}

void GLWidget::rotAtoms(QPoint delta)
{
    if(delta.isNull()){
        return;
    }
    float angle = delta.manhattanLength();
    auto axes = getAxes();
    Vec axis = delta.y() * axes[0] + -delta.x() * axes[1];
    if(curSel->getNat()){
        curSel->asFmt(AtomFmt::Bohr).modRotate(angle, axis, shift);
    }else{
        curStep->asFmt(AtomFmt::Bohr).modRotate(angle, axis, shift);
    }
    triggerUpdate(GUI::Change::atoms);
}

void GLWidget::shiftAtomsXY(QPoint delta)
{
    auto axes = Mat_trans(Mat_inv(getAxes()));
    Vec axis = delta.x() * axes[0] + delta.y() * axes[1];
    if(curSel->getNat()){
        curSel->asFmt(AtomFmt::Bohr).modShift(axis, 0.01f);
    }else{
        curStep->asFmt(AtomFmt::Bohr).modShift(axis, 0.01f);
    }
    triggerUpdate(GUI::Change::atoms);
}

void GLWidget::shiftAtomsZ(QPoint delta)
{
    auto axes = Mat_trans(Mat_inv(getAxes()));
    float fac = 0.01f * (delta.x() + delta.y());
    if(curSel->getNat()){
        curSel->asFmt(AtomFmt::Bohr).modShift(axes[2], fac);
    }else{
        curStep->asFmt(AtomFmt::Bohr).modShift(axes[2], fac);
    }
    triggerUpdate(GUI::Change::atoms);
}

void GLWidget::wheelEvent(QWheelEvent *e)
{
    zoomViewMat(e->angleDelta().y()>0?1.1f:0.9f);
    e->accept();
    update();
}

void GLWidget::mousePressEvent(QMouseEvent *e)
{
    e->accept();
    setFocus();
    if (!(e->buttons()&7)) {
        return;
    }
    rectPos = mousePos = e->pos();
    switch(mouseMode){
    case MouseMode::Camera:
        if(e->button() == Qt::MouseButton::RightButton){
            alignViewMat(alignDir::z);
            update();
        }
        break;
    case MouseMode::Select:
        if(e->button() == Qt::MouseButton::RightButton){
            curSel->setFilter(SelectionFilter{});
            triggerUpdate(GUI::Change::selection);
        }
        break;
    case MouseMode::Modify:
        if((e->button() == Qt::MouseButton::LeftButton) &&
           (e->modifiers() & (Qt::Modifier::CTRL|Qt::Modifier::SHIFT)) == 0u){
            auto idx = pickAtoms(mousePos, mousePos);
            if(idx.empty()){
                if(curSel->getNat()){
                    shift = curSel->getCom(AtomFmt::Bohr);
                }else{
                    shift = curStep->getCom(AtomFmt::Bohr);
                }
            }else{
                shift = curStep->asFmt(AtomFmt::Bohr)[idx.begin()->first].coord;
            }
        }
        break;
    case MouseMode::Bond:
        // nop
        break;
    }
}

void GLWidget::mouseMoveEvent(QMouseEvent *e)
{
    e->accept();
    if (!(e->buttons()&7)) {
        return;
    }
    QPoint delta = e->pos() - mousePos;
    switch(mouseMode){
    case MouseMode::Camera:
        if((e->buttons() & Qt::MouseButton::LeftButton) != 0u){
            if(e->modifiers() & Qt::Modifier::SHIFT){
                translateViewMat(delta.x(), -delta.y(), 0);
            }else{
                rotateViewMat(delta.x(), delta.y(), 0);
            }
            update();
        }else if((e->buttons() & Qt::MouseButton::MiddleButton) != 0u){
            translateViewMat(delta.x(), -delta.y(), 0);
            update();
        }
        rectPos = mousePos = e->pos();
        break;
    case MouseMode::Select:
        if((e->buttons() & Qt::MouseButton::RightButton) == 0u &&
                delta.manhattanLength() > 5){
            rectPos = e->pos();
            update();
        }
        break;
    case MouseMode::Modify:
        switch(e->buttons()){
        case Qt::MouseButton::LeftButton:
            if(e->modifiers() & Qt::Modifier::CTRL){
                shiftAtomsXY(delta);
            }else if(e->modifiers() & Qt::Modifier::SHIFT){
                shiftAtomsZ(delta);
            }else{
                rotAtoms(delta);
            }
            break;
        case Qt::MouseButton::MiddleButton:
            shiftAtomsXY(delta);
            break;
        case Qt::MouseButton::RightButton:
            shiftAtomsZ(delta);
            break;
        default:
            break;
        }
        rectPos = mousePos = e->pos();
        break;
    case MouseMode::Bond:
        // nop
        break;
    }
}

void GLWidget::mouseReleaseEvent(QMouseEvent *e)
{
    e->accept();
    switch(mouseMode){
    case MouseMode::Camera:
        break;
    case MouseMode::Select:
        if(e->button() != Qt::MouseButton::RightButton){
            bool add = (e->button() & Qt::MouseButton::MiddleButton) ||
                       (e->modifiers() & Qt::ControlModifier);
            SelectionFilter filter{};
            filter.mode = SelectionFilter::Mode::Index;
            if(add){
                const auto& origIndices = curSel->getIndices();
                filter.indices.insert(origIndices.begin(), origIndices.end());
            }
            auto idx = pickAtoms(mousePos, rectPos);
            if((idx.size() == 1) && (idx.begin()->second.size() == 1)){
                auto i = *idx.begin();
                auto pos = filter.indices.find(i.first);
                if(pos == filter.indices.end()){
                    // if not present, add single atom
                    filter.indices[i.first] = i.second;
                }else{
                    auto pbc = std::find(pos->second.begin(), pos->second.end(), i.second[0]);
                    if(pbc == pos->second.end()){
                        // atom present, add aditional periodic image
                        pos->second.push_back(i.second[0]);
                    }else if(pos->second.size() > 1){
                        // atom present, remove selected periodic image
                        pos->second.erase(pbc);
                    }else{
                        // atom present, no periodic images remaining, remove completely
                        filter.indices.erase(pos);
                    }
                }
            }else{
                // if area is selected, always merge sets
                for(const auto& p: idx){
                    auto& target = filter.indices[p.first];
                    for(auto& pbc: p.second){
                        if(std::find(target.begin(), target.end(), pbc) == target.end()){
                            target.push_back(pbc);
                        }
                    }
                }
            }
            curSel->setFilter(filter);
            rectPos = mousePos;
            triggerUpdate(GUI::Change::selection);
        }
        break;
    case MouseMode::Modify:
        //TODO: undo
        break;
    case MouseMode::Bond:
        // if we have manual bonds and picked exactly two atoms, toggle bond
        if(curStep->getBondMode() != BondMode::Manual) break;
        auto idx1 = pickAtoms(rectPos, rectPos);
        if(idx1.size() != 1 && idx1.begin()->second.size() != 1) break;
        auto idx2 = pickAtoms(e->pos(), e->pos());
        if(idx2.size() != 1 && idx2.begin()->second.size() != 1) break;
        auto at1 = *idx1.begin();
        auto at2 = *idx2.begin();
        DiffVec off_l = {static_cast<DiffVec::value_type>(at1.second.front()[0]-at2.second.front()[0]),
                         static_cast<DiffVec::value_type>(at1.second.front()[1]-at2.second.front()[1]),
                         static_cast<DiffVec::value_type>(at1.second.front()[2]-at2.second.front()[2]),
                        };
        DiffVec off_r = {static_cast<DiffVec::value_type>(-off_l[0]),
                         static_cast<DiffVec::value_type>(-off_l[1]),
                         static_cast<DiffVec::value_type>(-off_l[2]),
                        };
        // ignore bonds between one atom with itself
        if((at1.first == at2.first) && (off_l == DiffVec{0,0,0}))
            break;
        const auto& bonds = curStep->getBonds();
        // if bond is already present, delete it and return
        for(size_t i=0; i<bonds.size(); ++i){
            const auto& b = bonds[i];
            if(((b.at1 == at1.first) && (b.at2 == at2.first) && (b.diff == off_r)) ||
               ((b.at1 == at2.first) && (b.at2 == at1.first) && (b.diff == off_l))){
                curStep->delBond(i);
                triggerUpdate(GUI::Change::atoms);
                return;
            }
        }
        // create new bond
        curStep->newBond(at1.first, at2.first, off_r);
        triggerUpdate(GUI::Change::atoms);
        break;
    }
}
