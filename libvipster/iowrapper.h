#ifndef IOWRAPPER
#define IOWRAPPER

#include <map>
#include "molecule.h"
#include "ioplugin.h"
#include "ioplugins/xyz.h"
#include "ioplugins/pwinput.h"
#include "ioplugins/lmpinput.h"
#include "ioplugins/lmptrajec.h"

namespace Vipster{
    enum class  IOFmt{XYZ, PWI, LMP, DMP};
    const       std::map<IOFmt, IOPlugin const *const> IOPlugins{
                    {IOFmt::XYZ, &IO::XYZ},
                    {IOFmt::PWI, &IO::PWInput},
                    {IOFmt::LMP, &IO::LmpInput},
                    {IOFmt::DMP, &IO::LmpTrajec}
                };
    std::shared_ptr<IO::BaseData> readFile(std::string fn, IOFmt fmt);
    std::shared_ptr<IO::BaseData> readFile(std::string fn, IOFmt fmt, std::string name);
    bool        writeFile(std::string fn, IOFmt fmt, const Molecule &m, const IO::BaseParam* const p=nullptr);
}

#endif // IOWRAPPER

