#ifndef LIBVIPSTER_STEP_H
#define LIBVIPSTER_STEP_H

#include "stepbase.h"

#include <vector>
#include <memory>

namespace Vipster {

namespace detail{

/*
 * Basic serial Atom container
 *
 * Stores atom in separate vectors
 */
struct AtomList{
    template<bool isConst>
    class AtomView;
    using atom = AtomView<false>;
    using const_atom = AtomView<true>;
    template<bool isConst>
    using AtomIterator = detail::AtomIterator<AtomView, isConst>;
    using iterator = AtomIterator<false>;
    using const_iterator = AtomIterator<true>;

    AtomContext ctxt;
    // Coordinates
    std::vector<Vec> coordinates{};
    // Pointers to PeriodicTable entries
    std::vector<PeriodicTable::value_type*> elements{};
    // Properties
    std::vector<AtomProperties> properties{};
    // complete Step-interface
    size_t getNat() const noexcept {return elements.size();}

    AtomList(AtomFmt fmt)
        : ctxt{fmt} {}
    // specialize copy constructor because AtomList is supposed to own its cell
    AtomList(const AtomList &rhs)
        : ctxt{rhs.ctxt.fmt,
               rhs.ctxt.pte,
               std::make_shared<AtomContext::CellData>(*rhs.ctxt.cell)},
          coordinates{rhs.coordinates},
          elements{rhs.elements},
          properties{rhs.properties}
    {}
    AtomList(AtomList&&) = default;
    AtomList& operator=(const AtomList&) = default;
    AtomList& operator=(AtomList&&) = default;
    // enable creation from other Atom-sources
    template<typename T>
    AtomList(const T &rhs)
        : ctxt{rhs.ctxt.fmt,
               rhs.ctxt.pte,
               std::make_shared<AtomContext::CellData>(*rhs.ctxt.cell)},
          coordinates{rhs.coordinates},
          elements{rhs.elements},
          properties{rhs.properties}
    {}

    template<bool isConst>
    class AtomView
    {
        /* Wrapper class that wraps distributed SoA data into a singular Atom-interface
         *
         * should behave like a reference
         */
        Vec *v;
        PeriodicTable::value_type **elem;
        AtomProperties *prop;

        AtomView()
            : source{nullptr},
              v{nullptr},
              elem{nullptr},
              prop{nullptr}
        {}
        template<bool> friend class AtomView;
        class _Vec{
        public:
            _Vec(AtomView &a): a{a} {}
            // can only be explicitely constructed to reference an Atom
            _Vec(const _Vec&) = delete;
            // assigning changes the origin
            _Vec& operator=(const _Vec& rhs){
                return operator=(static_cast<const Vec&>(rhs));
            }
            _Vec& operator=(const Vec& rhs){
                *a.v = rhs;
                return *this;
            }
            // convert to regular Vec-reference
            operator Vec&() {return *a.v;}
            operator const Vec&() const {return *a.v;}
            // formatted Vec access
            Vec asFmt(const AtomContext &ctxt) const
            {
                return makeConverter(a.source->ctxt, ctxt)(static_cast<const Vec&>(*this));
            }
            // array access
            Vec::value_type& operator[](std::size_t i){
                return (*a.v)[i];
            }
            const Vec::value_type& operator[](std::size_t i) const {
                return (*a.v)[i];
            }
            // comparison
            bool operator==(const Vec& rhs) const {return *a.v == rhs;}
        private:
            AtomView &a;
        };
        class _Name{
        public:
            _Name(AtomView &a): a{a} {}
            // can only be explicitely constructed to reference an Atom
            _Name(const _Name&) =delete;
            // assigning a string makes this point to the
            // appropriate entry in the periodic table
            _Name& operator=(const _Name& rhs){
                *a.elem = &*a.source->ctxt.pte->find_or_fallback(rhs);
                return *this;
            }
            _Name& operator=(const std::string& rhs){
                *a.elem = &*a.source->ctxt.pte->find_or_fallback(rhs);
                return *this;
            }
            // convert to regular string-reference
            operator const std::string&() const {return (*a.elem)->first;}
            // comparison
            bool operator==(const std::string& rhs) const {return (*a.elem)->first == rhs;}
            // const char* access
            const char* c_str() const{
                return (*a.elem)->first.c_str();
            }
            // I/O
            friend std::ostream& operator<<(std::ostream &s, const _Name &n){
                return s << n.c_str();
            }
            friend std::istream& operator>>(std::istream &s, _Name &n){
                s >> n;
                return s;
            }
        private:
            AtomView &a;
        };
        class _Element{
        public:
            _Element(AtomView &a): a{a} {}
            // can only be explicitely constructed to reference an Atom
            _Element(const _Element& at) =delete;
            // Allow member-access via pointer-syntax
            Element* operator->() { return &(*a.elem)->second;}
            const Element* operator->() const{return &(*a.elem)->second;}
            // convert to reference
            operator const Element&() const {return (*a.elem)->second;}
            // comparison
            bool operator==(const Element& rhs) const { return (*a.elem)->second == rhs;}
        private:
            AtomView &a;
        };
        class _Properties{
        public:
            _Properties(AtomView &a): a{a} {}
            // can only be explicitely constructed to reference an Atom
            _Properties(const _Properties&) =delete;
            // like real reference, assigning changes the origin
            _Properties& operator=(const _Properties& rhs){
                *a.prop = static_cast<const AtomProperties&>(rhs);
                return *this;
            }
            _Properties& operator=(const AtomProperties& rhs){
                *a.prop = rhs;
                return *this;
            }
            // convert to regular AtomProperties-reference
            operator const AtomProperties&() const {return *a.prop;}
            // comparison
            bool operator==(const AtomProperties &rhs) const {return *a.prop == rhs;}
            // Allow pointer-syntax
            const AtomProperties* operator->() const {return a.prop;}
            AtomProperties* operator->() {return a.prop;}
        private:
            AtomView &a;
        };
    protected:
        using Source = AtomList;
        Source *source;
        AtomView& operator+=(ptrdiff_t i){
            v += i;
            elem += i;
            prop += i;
            return *this;
        }
        void pointTo(const AtomView& rhs){
            source = rhs.source;
            v = rhs.v;
            elem = rhs.elem;
            prop = rhs.prop;
        }
    public:
        // "Data", encapsulated in wrapper objects
        std::conditional_t<isConst, const _Vec, _Vec> coord{*this};
        std::conditional_t<isConst, const _Name, _Name> name{*this};
        std::conditional_t<isConst, const _Element, _Element> type{*this};
        std::conditional_t<isConst, const _Properties, _Properties> properties{*this};

        AtomView(AtomList &al,
                 size_t i)
            : source{&al},
              v{&al.coordinates[i]},
              elem{&al.elements[i]},
              prop{&al.properties[i]}
        {}
        // copying is templated to allow conversion to const
        // copy constructor creates new object pointing to same data
        AtomView(const AtomView &rhs)
            : source{rhs.source},
              v{rhs.v},
              elem{rhs.elem},
              prop{rhs.prop}
        {}
        template<bool B, bool t=isConst, typename = typename std::enable_if<t>::type>
        AtomView(const AtomView<B> &rhs)
            : source{rhs.source},
              v{rhs.v},
              elem{rhs.elem},
              prop{rhs.prop}
        {}
        // copy assignment changes data
        AtomView& operator=(const AtomView &rhs){
            coord = rhs.coord.asFmt(source->ctxt);
            name = rhs.name;
            properties = rhs.properties;
            return *this;
        }
        template<bool B, bool t=isConst, typename = typename std::enable_if<t>::type>
        AtomView& operator=(const AtomView<B> &rhs){
            coord = rhs.coord.asFmt(source->ctxt);
            name = rhs.name;
            properties = rhs.properties;
            return *this;
        }
        template<bool B, template<bool> typename AV>
        AtomView& operator=(const AV<B> &rhs){
            coord = rhs.coord.asFmt(source->ctxt);
            name = rhs.name;
            properties = rhs.properties;
            return *this;
        }
        virtual ~AtomView() = default;

        bool operator==(const AtomView &rhs) const noexcept{
            return std::tie(coord, name, properties)
                    ==
                   std::tie(rhs.coord, rhs.name, rhs.properties);
        }
        bool operator!=(const AtomView &rhs) const noexcept{
            return !operator==(rhs);
        }
    };
};

}

/*
 * Main Step-class
 *
 * with AtomList as source, this is the main class to use for atom-storage
 */

class Step: public StepMutable<detail::AtomList>
{
    friend class Molecule;
public:
    Step(AtomFmt at_fmt=AtomFmt::Angstrom,
         const std::string &comment="");
    Step(const Step& s);
    Step(Step&& s);
    Step& operator=(const Step& s);
    Step& operator=(Step&& s);
    template<typename T>
    Step(const StepConst<T>& s)
        : StepMutable{std::make_shared<detail::AtomList>(s.getFmt()),
                      std::make_shared<BondList>(),
                      std::make_shared<std::string>(s.getComment())}
    {
        // copy Cell
        if(s.hasCell()){
            setCellDim(s.getCellDim(AtomFmt::Bohr), AtomFmt::Bohr);
            setCellVec(s.getCellVec());
        }
        // copy Atoms
        newAtoms(s);
        // copy Bonds
        for(const auto& b: s.getBonds()){
            addBond(b.at1, b.at2, b.diff, b.type ? b.type->first : "");
        }
    }

    // Atoms
    void    newAtom(std::string name="",
                    Vec coord=Vec{},
                    AtomProperties prop=AtomProperties{});
    template<template<bool> typename T, bool B>
    void    newAtom(const T<B>& at){
        newAtom();
        back() = at;
    }
    void    newAtoms(size_t i);
    template<typename T>
    void    newAtoms(const StepConst<T>& s)
    {
        const size_t nat = this->getNat() + s.getNat();
        // reserve space for new atoms
        atom_source& al = *this->atoms;
        al.coordinates.reserve(nat);
        al.elements.reserve(nat);
        al.properties.reserve(nat);
        // TODO: only place where getAtoms().ctxt is needed, refactor?
        auto fmt = detail::makeConverter(s.getAtoms().ctxt, atoms->ctxt);
        for(const auto &at: s){
            al.coordinates.push_back(fmt(at.coord));
            al.elements.push_back(&*al.ctxt.pte->find_or_fallback(at.name));
            al.properties.push_back(at.properties);
        }
    }
    void    delAtom(size_t i);
    template<typename T>
    void delAtoms(StepConst<detail::Selection<T>>& s)
    {
        const auto& idx = s.getAtoms().indices;
        for(auto it = idx.rbegin(); it != idx.rend(); ++it)
        {
            delAtom(it->first);
        }
        s = select({});
    }

    // Cell
    void enableCell(bool val) noexcept;
    void setCellDim(double cdm, AtomFmt fmt, bool scale=false);
    void setCellVec(const Mat &vec, bool scale=false);

    // Modifier functions
    void modWrap();
    void modCrop();
    void modMultiply(size_t x, size_t y, size_t z);
    void modAlign(uint8_t step_dir, uint8_t target_dir);
    void modReshape(Mat newMat, double newDim, AtomFmt Fmt);
};

}
#endif // LIBVIPSTER_STEP_H
