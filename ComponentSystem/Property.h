#ifndef COMSYS_ PROPERTY_H
#define COMSYS_PROPERTY_H

namespace CmSys
{
  typedef unsigned int PropertyID;
  static constexpr PropertyID PropertyNull = 0;

  // Static ID of primitive properties
  enum PrimitivePropertyID : PropertyID
  {
    Prop_Prim_NULL = PropertyNull,
    Prop_Prim_Bool,
    Prop_Prim_UInt,
    Prop_Prim_Int,
    Prop_Prim_Float,
    Prop_Prim_Double,
    Prop_Prim_NEXT, // Count
  };

  // Static ID of references to external properties
  enum LinkPropertyID : PropertyID
  {
    Prop_Link_NULL = PropertyNull,
    Prop_Link_Any  = Prop_Prim_NEXT,
    Prop_Link_Inherits,
    Prop_Link_Strict,
    Prop_Link_Raw,
    Prop_Link_NEXT
  };

  // Static ID of  properties
  enum BundlePropertyID : PropertyID
  {
    Prop_Bndl_NULL = PropertyNull,
    Prop_Bndl_Array = Prop_Link_NEXT,
    Prop_Bndl_NEXT
  };

  // Static ID of the start of each property group
  enum GroupPropertyID : PropertyID
  {
    Prop_Group_NULL      = PropertyNull,
    Prop_Group_Primitive = Prop_Prim_Bool,
    Prop_Group_Link      = Prop_Link_Any,
    Prop_Group_Bundle    = Prop_Bndl_Array,
    Prop_Group_Object    = Prop_Bndl_NEXT // The rest of the IDs are objects
  };

} // Component System


#endif