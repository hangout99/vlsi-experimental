#ifndef __HIGH_BASEDESIGNITEM_H__
#define __HIGH_BASEDESIGNITEM_H__

#include "LDesign.h"
#include "stdTypes.h"
#include "Configuration.h"
#include "Logging.h"
#include "HEnumerator.h"
#include "Event.h"
#include "Grow.h"

class HDesign;

struct HDesignPointer
{
  IDType __uid;
};

template<class Element>
class HBaseCollection
{
protected:
  LDesign* m_ld;
  HDesign* m_hd;
  bool m_initialized;

  HBaseCollection(HDesign* hd): m_hd(hd), m_ld(&hd->_Design), m_initialized(false)
  {
  }

public:
  Element Null() const {return Element();}

  template<class Base>
  friend LDesign* ToLow(HBaseCollection<Base>*);

  typedef Element ItemType;
  typedef typename Element::Properties Property;
};

template<class Ptr>
inline IDType ToID(const Ptr& ptr) { return ((HDesignPointer&)ptr).__uid;}

template<class Ptr>
inline void FromID(const Ptr& ptr, IDType id) { ((HDesignPointer&)ptr).__uid = id;}

template<class Ptr>
inline IDType& __GetID(const Ptr& ptr) { return ((HDesignPointer&)ptr).__uid;}

template<class Ptr>
inline bool IsNull(const Ptr& ptr) { return ToID(ptr) == 0;}

template<class Base>
inline LDesign* ToLow(HBaseCollection<Base>* c) {return c->m_ld;}

template<class Ptr, class Arg>
inline Ptr __ConstructPtr(Arg base) { return *((Ptr*)&base); }

/***** Standard macro templates for core classes *****/
//abstract getters/setter definition
#define GETTERS_SETTERS_DEFINITION() \
  template<Property prop, class RetType> \
  RetType Get(ItemType elem) const \
  { Property_is_not_implemented } \
  \
  template<Property prop, class ArgType> \
  void Set(ItemType elem, const ArgType& value) \
  { Property_is_readonly_or_is_not_implemented } \
  \
  template<Property prop> \
  double GetDouble(ItemType elem) const {return Get<prop, double>(elem);} \
  \
  template<Property prop> \
  int GetInt(ItemType elem) const {return Get<prop, int>(elem);} \
  \
  template<Property prop> \
  bool GetBool(ItemType elem) const {return Get<prop, bool>(elem);} \
  \
  template<Property prop> \
  string GetString(ItemType elem) const {return Get<prop, string>(elem);}

//shortcut for argument ID
#define ARGID (::ToID(arg))

//getters macrodefinitions
#define GETTER(RetType, Property) \
  template<> RetType Get<Property, RetType>(ItemType arg) const

#define IGETTER(RetType, Property, Collection) \
  template<> inline RetType Collection::Get<Property, RetType>(Collection::ItemType arg) const

#define GETTERA(RetType, Property, ArrayName) \
  GETTER(RetType, Property) \
  { return ArrayName[ARGID]; }

#define GETTERA2(RetType, Property, ArrayName) \
  GETTER(RetType, Property) \
  { return ::__ConstructPtr<RetType, IDType>(ArrayName[ARGID]); }

//setters macro definitions
#define SETTER(PropType, Property) \
  template<> void Set<Property>(ItemType arg, const PropType& value)

#define SETTERA(PropType, Property, ArrayName) \
  SETTER(PropType, Property) \
  { ArrayName[ARGID] = value; }

#define SETTERA2(PropType, Property, ArrayName) \
  SETTER(PropType, Property) \
  { ArrayName[ARGID] = ::ToID(value); }

#define ISETTER(ArgType, Property, Collection) \
  template<> inline void Collection::Set<Property, ArgType>(Collection::ItemType arg, const ArgType& value)

//dual getter & setter definition
#define PROPERTYA(PropType, Property, ArrayName) \
GETTERA(PropType, Property, ArrayName)\
SETTERA(PropType, Property, ArrayName)

#define PROPERTYA2(PropType, Property, ArrayName) \
GETTERA2(PropType, Property, ArrayName)\
SETTERA2(PropType, Property, ArrayName)

//wrapper's getters macrodefinitions
#define GETTERWEX(RetType, GetterName) \
RetType GetterName() const

#define GETTERW(RetType, PropertyShort, GetterName) \
GETTERWEX(RetType, GetterName) { return m_Parent->Get<BaseType::PropertyShort, RetType>(*this); }

#define GETTERWD(RetType, PropertyShort) \
GETTERW(RetType, PropertyShort, PropertyShort)

#define IGETTERW(RetType, WrapperType, PropertShort, GetterName) \
inline RetType WrapperType::GetterName() const \
{ return m_Parent->Get<WrapperType::BaseType::PropertShort, RetType>(*this); }

#define IGETTERWD(RetType, WrapperType, PropertShort) \
  IGETTERW(RetType, WrapperType, PropertShort, PropertShort)

//wrapper's setters macrodefinitions
#define SETTERWEX(ArgType, SetterName) \
void Set##SetterName(const ArgType& value)

#define SETTERW(ArgType, PropertyShort, SetterName) \
SETTERWEX(ArgType, SetterName) { m_Parent->Set<BaseType::PropertyShort>(*this, value); }

#define SETTERWD(ArgType, PropertyShort) \
SETTERW(ArgType, PropertyShort, PropertyShort)

#define ISETTERW(ArgType, WrapperType, PropertShort, SetterName) \
inline void WrapperType::Set##SetterName(const ArgType& value) \
{ m_Parent->Set<WrapperType::BaseType::PropertShort, ArgType>(*this, value); }

#define ISETTERWD(ArgType, WrapperType, PropertShort) \
  ISETTERW(ArgType, WrapperType, PropertShort, PropertShort)

//wrapper's dual getters & setters macrodefinitions
#define PROPERTYWD(ArgType, PropertyShort) \
GETTERWD(ArgType, PropertyShort) \
SETTERWD(ArgType, PropertyShort)

#define PROPERTYW(ArgType, PropertyShort, SetterName) \
GETTERW(ArgType, PropertyShort, SetterName) \
SETTERW(ArgType, PropertyShort, SetterName)


//wrapper opening and ending
#define BEGINWRAPPER(wrapper_type, collection_type) \
class wrapper_type : public collection_type::ItemType { \
public: \
  typedef wrapper_type SelfType; \
  typedef collection_type CollectionType; \
  typedef collection_type::ItemType BaseType; \
  friend class CollectionType; \
  \
protected: \
  CollectionType *m_Parent; \
  wrapper_type(CollectionType *aPr, IDType aId) : BaseType(aId), m_Parent(aPr) \
  { } \
public: \
  explicit wrapper_type(CollectionType *aPr) : BaseType(), m_Parent(aPr) \
  { } \
  \
  SelfType& operator= (BaseType value) \
  { ::FromID(*this, ::ToID(value)); return *this; }

#define ENDWRAPPER(wrapper_type) \
}; \
inline wrapper_type wrapper_type::CollectionType::MakeWrapper(wrapper_type::BaseType item) \
{ return wrapper_type(this, ::ToID(item)); } 


//main design item opening and ending
#define BEGINITEM(item_name, wrapper_name) \
class wrapper_name; \
class item_name: protected HDesignPointer \
{ \
protected: \
  item_name(IDType aId) \
  { ::FromID(*this, aId); } \
public: \
  item_name() { ::FromID(*this, 0); } \
  typedef wrapper_name WrapperType; \
  typedef item_name SelfType; \
  bool operator == (const item_name& other) { return ::ToID(other) == ::ToID(*this); } \
  bool operator != (const item_name& other) { return ::ToID(other) != ::ToID(*this); }

#define ENDITEM(ItemName) \
};


//design collection opening and ending
#define BEGINHCOLLECTION(collection_type, base_type) \
class collection_type: public HBaseCollection<base_type> \
{ \
public: \
  collection_type(HDesign *aHd); \
  \
  typedef collection_type SelfType; \
  typedef base_type ItemType; \
  typedef HBaseCollection<ItemType> BaseType; \
  typedef ItemType::WrapperType WrapperType; \
  \
  WrapperType MakeWrapper(ItemType item); \
  HDesign& Parent();

#define ENDHCOLLECTION(collection_type)\
};

#define COLLECTIONCONSTRUCTOR(collection_type) \
collection_type::collection_type(HDesign *aHd): collection_type::BaseType(aHd) {} \
HDesign& collection_type::Parent() { return *m_hd; }

#define CHECKIFINITIALIZED() {if (m_initialized) return; m_initialized = true;}


#endif //__HIGH_BASEDESIGNITEM_H__