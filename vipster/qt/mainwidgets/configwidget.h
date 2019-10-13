#ifndef CONFIGWIDGET_H
#define CONFIGWIDGET_H

#include "../basewidget.h"
#include "../configwidgets.h"
#include "io.h"

namespace Ui {
class ConfigWidget;
}

class ConfigWidget : public BaseWidget
{
    Q_OBJECT

public:
    explicit ConfigWidget(QWidget *parent = nullptr);
    ~ConfigWidget() override;
    std::vector<std::pair< Vipster::IOFmt, std::unique_ptr<Vipster::IO::BaseConfig>>> configs;
    void registerConfig(std::unique_ptr<Vipster::IO::BaseConfig>&& data);
    void clearConfigs();
    Vipster::IOFmt curFmt;
    Vipster::IO::BaseConfig* curConfig{nullptr};

private slots:
    void on_configSel_currentIndexChanged(int index);

    void on_helpButton_clicked();

private:
    Ui::ConfigWidget *ui;
    std::map<Vipster::IOFmt, ConfigBase*> formats;
};

#endif // CONFIGWIDGET_H