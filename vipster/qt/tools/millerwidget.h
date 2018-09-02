#ifndef MILLERWIDGET_H
#define MILLERWIDGET_H

#include <QWidget>
#include "../mainwindow.h"
#include "../../common/meshdata.h"
#include "molecule.h"

namespace Ui {
class MillerWidget;
}

class MillerWidget : public BaseWidget
{
    Q_OBJECT

public:
    explicit MillerWidget(QWidget *parent = nullptr);
    ~MillerWidget() override;
    void updateWidget(uint8_t) override;

private slots:
    void updateIndex(int idx);
    void updateOffset(double off);
    void on_pushButton_toggled(bool checked);

private:
    Ui::MillerWidget *ui;
    struct MillerPlane{
        bool display;
        std::array<uint8_t, 3> hkl;
        Vipster::Vec offset;
        Vipster::GUI::MeshData gpu_data;
    };
    bool active{false};
    Vipster::StepProper* curStep{nullptr};
    MillerPlane* curPlane{nullptr};
    std::map<Vipster::StepProper*, MillerPlane> planes;
};

#endif // MILLERWIDGET_H
