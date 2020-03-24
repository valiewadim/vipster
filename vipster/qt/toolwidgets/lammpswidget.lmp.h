#ifndef LAMMPSWIDGET_LMP_H
#define LAMMPSWIDGET_LMP_H

#include "basewidget.h"

namespace Ui {
class LammpsWidget;
}

class LammpsWidget : public BaseWidget
{
    Q_OBJECT

public:
    explicit LammpsWidget(QWidget *parent = nullptr);
    ~LammpsWidget();

private slots:
    void on_runButton_clicked();
    void on_helpButton_clicked();

    void on_ffPrepare_clicked();

private:
    Ui::LammpsWidget *ui;
};

#endif // LAMMPSWIDGET_LMP_H
