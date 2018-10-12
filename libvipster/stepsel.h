#ifndef LIBVIPSTER_STEPSEL_H
#define LIBVIPSTER_STEPSEL_H

#include "stepmutable.h"

namespace Vipster {
// Forward declarations
class Step;
template<typename T>
class SelectionBase;
using StepSelection = SelectionBase<Step>;
using StepSelConst = SelectionBase<const Step>;

/*
 * Wrap multiple filter criteria without polymorphism
 * also represents hierarchically coupled filter chains
 * Target grammar:
 * Keywords are case insensitive, whereas atom-types are case-sensitive
 *
 * Filter ::= (Criterion, {Coupling, Filter}) | ("(", Filter, ")"));
 * Criterion ::= ["not "], (TypeCrit | IdxCrit | PosCrit | CoordCrit);
 * Coupling ::= ["!"], ("|" | "&" | "^");
 *
 * TypeCrit ::= "type ", (Type | TypeList);
 * TypeList ::= "[", Type, {(" ", Type)}, "]";
 * Type ::= NonWhiteSpace, {NonWhiteSpace};
 *
 * IdxCrit ::= "index " , (IdxList | IdxRange);
 * IdxList ::= ( "[", IdxRange, {(" " IdxRange)}, "]");
 * IdxRange ::= ( Integer, "-", Integer) | Integer;
 *
 * PosCrit ::= "pos ", Direction, Format, CompOp, Float;
 * Direction ::= "x" | "y" | "z";
 * Format ::= "a" | "b" | "c" | "d";
 * CompOp ::= ">" | "<";
 *
 * CoordCrit ::= "coord ", CompEqOp, Integer;
 * CompEqOp ::= "=" | CompOp;
 */
struct SelectionFilter;
std::ostream& operator<<(std::ostream& os, const SelectionFilter& filter);
std::istream& operator>>(std::istream& is, SelectionFilter& filter);
std::vector<size_t> evalFilter(const Step& step, SelectionFilter& filter);

struct SelectionFilter{
    SelectionFilter() = default;
    SelectionFilter(const SelectionFilter& f) {
        *this = f;
    }
    SelectionFilter& operator=(const SelectionFilter& f){
        mode = f.mode; op = f.op | Op::UPDATE;
        pos = f.pos; posVal = f.posVal;
        coord = f.coord; coordVal = f.coordVal;
        indices = f.indices; types = f.types;
        if(f.subfilter){
            subfilter = std::make_unique<SelectionFilter>(*f.subfilter);
        }
        if(f.groupfilter){
            groupfilter = std::make_unique<SelectionFilter>(*f.groupfilter);
        }
        return *this;
    }
    SelectionFilter& operator=(SelectionFilter&& f){
        mode = f.mode; op = f.op | Op::UPDATE;
        pos = f.pos; posVal = f.posVal;
        coord = f.coord; coordVal = f.coordVal;
        indices = f.indices; types = f.types;
        subfilter = std::move(f.subfilter);
        groupfilter = std::move(f.groupfilter);
        return *this;
    }
    std::string toStr() const
    {
        std::stringstream ss{};
        ss << *this;
        return ss.str();
    }
    enum class Mode:uint8_t{None, Index, Type, Coord, Pos, Group};
    enum Op{NONE=0x0, NOT=0x1, // first bit negates own op
            PAIR=0x2, NOT_PAIR=0x4, // second bit activates coupling, third bit negates
            AND=0x2, NAND=0x6,
            OR=0xA, NOR=0xE,
            XOR=0x12, XNOR=0x16,
            PAIR_MASK=0x1E,
            UPDATE=0x80};
    enum Pos{BOHR=0x0, ANG=0x1, CRYS=0x2, CDM=0x3, FMT_MASK=0x3,// 2 bits for format
             P_GT=0x0, P_LT=0x4, P_CMP_MASK=0x4,  // 1 bit for comp direction
             X=0x0, Y=0x8, Z=0x10, DIR_MASK=0x18, // 2  bits for space direction
            };
    enum Coord{C_GT=0x0, C_EQ=0x1, C_LT=0x2, C_CMP_MASK=0x3};
    Mode mode;
    uint8_t op{Op::UPDATE};
    uint8_t pos;
    uint8_t coord;
    float posVal;
    size_t coordVal;
    std::set<size_t> indices;
    std::set<std::string> types;
    std::unique_ptr<SelectionFilter> groupfilter{nullptr};
    std::unique_ptr<SelectionFilter> subfilter{nullptr};
};

/*
 * Selection container
 *
 * contains indices of selected atoms in Step-like
 */
template<typename T>
class AtomSelIterator;
template<typename T>
struct AtomSelection{
    using iterator = AtomSelIterator<T>;
    using constIterator = AtomSelIterator<const T>;

    std::vector<size_t> indices;
    T*                  step;
    SelectionFilter     filter;
};

/*
 * Iterator for Atom selection
 *
 * dereferences selection-indices
 */
template<typename T>
class AtomSelIterator: private decltype(std::declval<T>().begin())
{
private:
    using Base = decltype(std::declval<T>().begin());
public:
    AtomSelIterator(std::shared_ptr<AtomSelection<T>> selection,
                    AtomFmt, size_t idx)
    //TODO: introduce a terminal-object (when c++17 is ready?)
        : Base{selection->step->begin()},
          selection{selection}, idx{idx}
    {
        static_cast<Base*>(this)->operator+=(idx);
    }
    AtomSelIterator& operator++(){
        return operator+=(1);
    }
    AtomSelIterator& operator+=(size_t i){
        idx += i;
        auto diff = selection->indices[idx] - selection->indices[idx-i];
        static_cast<Base*>(this)->operator+=(diff);
        return *this;
    }
    AtomSelIterator operator+(size_t i){
        AtomSelIterator copy{*this};
        return copy+=i;
    }
    decltype(auto)  operator*() const {
        return static_cast<const Base*>(this)->operator*();
    }
    decltype(auto)  operator->() const {
        return static_cast<const Base*>(this)->operator->();
    }
    bool    operator==(const AtomSelIterator& rhs) const noexcept{
        return (selection == rhs.selection) && (idx == rhs.idx);
    }
    bool    operator!=(const AtomSelIterator& rhs) const noexcept{
        return !(*this == rhs);
    }
    size_t getIdx() const noexcept{
        return idx;
    }
private:
    std::shared_ptr<AtomSelection<T>> selection;
    size_t idx;
};

/*
 * Basic Selection-class template
 *
 * Instantiation of Bond- and Cell-getters with AtomSelection as Atom-source
 * Shall be instanced with `Step` or `const Step` as template argument
 */

template<typename T>
class SelectionBase: public StepMutable<AtomSelection<T>>
{
public:
    SelectionBase(std::shared_ptr<PseMap> p, AtomFmt f,
                  T* s, SelectionFilter sf,
                  std::shared_ptr<BondList> b,
                  std::shared_ptr<CellData> c, std::shared_ptr<std::string> co)
        : StepMutable<AtomSelection<T>>{p, f,
            std::make_shared<AtomSelection<T>>(),
//            std::make_shared<AtomSelection<T>>(evalFilter(*s, sf), s, sf),
                                        b, c, co}
    {
        this->atoms->step = s;
        setFilter(sf);
    }
    SelectionBase(std::shared_ptr<PseMap> p, AtomFmt f,
                  T* s, std::string sf,
                  std::shared_ptr<BondList> b,
                  std::shared_ptr<CellData> c, std::shared_ptr<std::string> co)
        : StepMutable<AtomSelection<T>>{p, f,
            std::make_shared<AtomSelection<T>>(),
                                        b, c, co}
    {
        this->atoms->step = s;
        setFilter(sf);
    }
    SelectionBase(const SelectionBase& s)
        : StepMutable<AtomSelection<T>>{s.pse,
                        s.at_fmt,
                        std::make_shared<AtomSelection<T>>(*s.atoms),
                        std::make_shared<BondList>(*s.bonds),
                        s.cell,
                        std::make_shared<std::string>(*s.comment)}
    {}
    SelectionBase& operator=(const SelectionBase& s)
    {
        this->pse = s.pse;
        this->at_fmt = s.at_fmt;
        *this->bonds = *s.bonds;
        this->cell = s.cell;
        *this->comment = *s.comment;
        *this->atoms = *s.atoms;
        return *this;
    }

    const std::vector<size_t>& getIndices() const noexcept
    {
        return this->atoms->indices;
    }

    const SelectionFilter& getFilter() const noexcept
    {
        return this->atoms->filter;
    }
    void setFilter(std::string filter)
    {
        auto fs = std::stringstream{filter};
        fs >> this->atoms->filter;
    }
    void setFilter(SelectionFilter filter)
    {
        this->atoms->filter = std::move(filter);
    }
    void evaluateFilter() const
    {
        this->atoms->indices = evalFilter(this->atoms->step, this->atoms->filter);
    }
};

template<>
size_t StepConst<AtomSelection<Step>>::getNat() const noexcept;

template<>
void StepConst<AtomSelection<Step>>::evaluateCache() const;

}

#endif // LIBVIPSTER_STEPSEL_H
