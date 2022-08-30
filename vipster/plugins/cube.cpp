#include "cube.h"

using namespace Vipster;

IOTuple CubeParser(const std::string& name, std::istream &file){
    Molecule mol{name, 1};
    DataList data;
    Step &s = mol.getStep(0);
    s.setFmt(AtomFmt::Bohr);

    std::string line, buf;
    std::stringstream lstream;

    // two comment lines
    std::getline(file, buf);
    std::getline(file, line);
    if(!buf.empty() && !line.empty()){
        s.setComment(buf + " | " + line);
    }else if(!buf.empty()){
        s.setComment(buf);
    }else if(!line.empty()){
        s.setComment(line);
    }

    std::getline(file, line);
    lstream.str(line);
    long tmp;
    bool mo_mode{false};
    size_t nat, nval;
    Vec origin;
    Mat cell;
    SizeVec extent;
    lstream >> tmp >> origin[0] >> origin[1] >> origin[2];
    if(lstream.fail()){
        throw IOError("Cube: failed to parse nat/origin");
    }
    nat = static_cast<size_t>(std::abs(tmp));
    if(tmp<0){
        mo_mode = true;
    }
    s.newAtoms(static_cast<size_t>(nat));
    lstream >> nval;
    if(lstream.fail()){
        nval = 1;
    }
    for(size_t i=0; i<3; ++i){
        file >> tmp >> cell[i][0] >> cell[i][1] >> cell[i][2];
        extent[i] = static_cast<size_t>(std::abs(tmp));
        if(tmp<0){
            cell[i] *= extent[i]*invbohr;
        }else{
            cell[i] *= extent[i];
        }
    }
    s.setCellDim(1, AtomFmt::Bohr);
    s.setCellVec(cell);
    for(auto& at: s){
        file >> at.name >> at.properties->charge
             >> at.coord[0] >> at.coord[1] >> at.coord[2];
    }
    if(mo_mode){
        // twice to consume left-over '\n'
        std::getline(file, line);
        std::getline(file, line);
        lstream.str(line);
        lstream.clear();
        size_t nmo;
        lstream >> nmo;
        std::vector<size_t> mo_indices(nmo);
        std::vector<DataGrid3D_f> grids;
        for(size_t i=0; i<nmo; ++i){
            lstream >> mo_indices[i];
            grids.emplace_back(extent);
            grids.back().origin = origin;
            grids.back().cell = cell;
            grids.back().name = name + " MO: " + std::to_string(mo_indices[i]);
        }
        for(size_t i=0; i<extent[0]; ++i){
            for(size_t j=0; j<extent[1]; ++j){
                for(size_t k=0; k<extent[2]; ++k){
                    for(auto& grid: grids){
                        file >> grid(i,j,k);
                    }
                }
            }
        }
        for(auto&& grid: grids){
            data.emplace_back(std::make_unique<const DataGrid3D_f>(std::move(grid)));
        }
    }else{
        if(nval==1){
            DataGrid3D_f grid{extent};
            grid.origin = origin;
            grid.cell = cell;
            grid.name = name;
            for(size_t i=0; i<extent[0]; ++i){
                for(size_t j=0; j<extent[1]; ++j){
                    for(size_t k=0; k<extent[2]; ++k){
                        file >> grid(i,j,k);
                    }
                }
            }
            data.emplace_back(std::make_unique<const DataGrid3D_f>(std::move(grid)));
        }else if(nval==4){
            DataGrid3D_f density{extent};
            DataGrid3D_v gradient{extent};
            density.origin = origin;
            density.cell = cell;
            density.name = name;
            gradient.origin = origin;
            gradient.cell = cell;
            gradient.name = name + " Gradient";
            for(size_t i=0; i<extent[0]; ++i){
                for(size_t j=0; j<extent[1]; ++j){
                    for(size_t k=0; k<extent[2]; ++k){
                        file >> density(i,j,k);
                        auto& tmp = gradient(i,j,k);
                        file >> tmp[0] >> tmp[1] >> tmp[2];
                    }
                }
            }
            data.emplace_back(std::make_unique<const DataGrid3D_f>(std::move(density)));
            data.emplace_back(std::make_unique<const DataGrid3D_v>(std::move(gradient)));
        }else{
            throw IOError("Cube: Only scalar or scalar+gradient grids are supported");
        }
    }

    return {std::move(mol), std::optional<Parameter>{}, std::move(data)};
}

const Plugin Plugins::Cube =
{
    "Gaussian Cube file",
    "cube",
    "cube",
    &CubeParser
};
