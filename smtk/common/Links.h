//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#ifndef smtk_common_Links_h
#define smtk_common_Links_h

#include <boost/multi_index/global_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include <functional>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace smtk
{
namespace common
{
template <typename id_type, typename left_type, typename right_type, typename base_type>
class Links;

/// A Link is an object that associates (or "links") two pieces of information
/// together. Since multiple links can exist between the same objects, links
/// have an id for unique identification. Links also have a field to describe
/// their role. Since roles are finite (and frequently few), Links store a
/// reference to a string held in the containing Links class.
template <typename id_type, typename left_type, typename right_type, typename base_type>
struct Link : base_type
{
  Link(const base_type& base_, const id_type& id_, const left_type& left_, const right_type& right_,
    const std::string& role_)
    : base_type(base_)
    , id(id_)
    , left(left_)
    , right(right_)
    , role(role_)
  {
  }

  Link(base_type&& base_, const id_type& id_, const left_type& left_, const right_type& right_,
    const std::string& role_)
    : base_type(base_)
    , id(id_)
    , left(left_)
    , right(right_)
    , role(role_)
  {
  }

  virtual ~Link() {}

  id_type id;
  left_type left;
  right_type right;
  const std::string& role;
};

namespace detail
{
struct NullLinkBase
{
};

/// Tags for access.
struct Id
{
};
struct Left
{
};
struct Right
{
};
struct Role
{
};

/// A wrapper class for accessing the Role, since const reference fields are not
/// trivially accessible within the multiindex container.
template <typename id_type, typename left_type, typename right_type, typename base_type>
inline const std::string& role(const Link<id_type, left_type, right_type, base_type>& link)
{
  return link.role;
}

using namespace boost::multi_index;

/// A multi-index container that has unique id indexing and non-unique left,
/// right and role indexing. A link_type is also expected; users are optionally
/// able to use template classes that inherit from Link and augment its storage
/// and utility.
template <typename id_type, typename left_type, typename right_type, typename base_type>
using LinkContainer = boost::multi_index_container<
  Link<id_type, left_type, right_type, base_type>,
  indexed_by<
    ordered_unique<tag<Id>, member<Link<id_type, left_type, right_type, base_type>, id_type,
                              &Link<id_type, left_type, right_type, base_type>::id> >,
    ordered_non_unique<tag<Left>, member<Link<id_type, left_type, right_type, base_type>, left_type,
                                    &Link<id_type, left_type, right_type, base_type>::left> >,
    ordered_non_unique<tag<Right>,
      member<Link<id_type, left_type, right_type, base_type>, right_type,
                         &Link<id_type, left_type, right_type, base_type>::right> >,
    ordered_non_unique<tag<Role>,
      global_fun<const Link<id_type, left_type, right_type, base_type>&, const std::string&,
                         &role<id_type, left_type, right_type, base_type> > > > >;

/// Traits classes for Links. We key off of the tags to return sane responses
/// in the Links class.
template <typename id_type, typename left_type, typename right_type, typename base_type,
  typename tag>
struct LinkTraits;

template <typename id_type, typename left_type, typename right_type, typename base_type>
struct LinkTraits<id_type, left_type, right_type, base_type, Left>
{
  typedef Link<id_type, left_type, right_type, base_type> Link_;
  typedef Right OtherTag;
  typedef left_type type;
  typedef right_type other_type;
  static const type& value(const Link_& a) { return a.left; }
  static void setValue(Link_& a, const type& v) { a.left = v; }
};

template <typename id_type, typename left_type, typename right_type, typename base_type>
struct LinkTraits<id_type, left_type, right_type, base_type, Right>
{
  typedef Link<id_type, left_type, right_type, base_type> Link_;
  typedef Left OtherTag;
  typedef right_type type;
  typedef left_type other_type;
  static const type& value(const Link_& a) { return a.right; }
  static void setValue(Link_& a, const type& v) { a.right = v; }
};

template <typename id_type, typename left_type, typename right_type, typename base_type>
struct LinkTraits<id_type, left_type, right_type, base_type, Role>
{
  typedef Link<id_type, left_type, right_type, base_type> Link_;
  typedef std::string type;
  static const type& value(const Link_& a) { return a.role; }
  static void setValue(Link_&, const type&) {}
};
}

/// The Links class represents a collection of Link-s. It has a set-like
/// interface that permits insertion, erasure, iteration, find, etc.
/// Additionally, template methods that accept the Left, Right and Role tags
/// facilitate return values that isolate the Left, Right and Role values (as
/// though the container were a container of just that type). Internally, the
/// links are held in a multiindex array to facilitate indexing according to the
/// Id (default), Left, Right and Role values of the links.
template <typename id_type, typename left_type = id_type, typename right_type = left_type,
  typename base_type = detail::NullLinkBase>
class Links : public detail::LinkContainer<id_type, left_type, right_type, base_type>
{
  using Parent = detail::LinkContainer<id_type, left_type, right_type, base_type>;
  template <typename tag>
  using LinkTraits = detail::LinkTraits<id_type, left_type, right_type, base_type, tag>;

public:
  static const std::string undefinedRole;

  virtual ~Links() {}

  /// The "Left", "Right" and "Role" tags facilitate access to views into the
  /// container that are sorted according to the left, right or role values,
  /// respectively.
  using Left = detail::Left;
  using Right = detail::Right;
  using Role = detail::Role;

  /// We expose a subset of the base class's types and methods because we use
  /// them for untagged interaction (i.e. methods that do not use a tag) with
  /// the container.
  using iterator = typename Parent::iterator;
  using Link = typename Parent::value_type;
  using LinkBase = base_type;
  using Parent::insert;
  using Parent::size;

  /// Insertion into the container is performed by passing values for the
  /// base_type object, link id, left value, right value, and role. Since there
  /// are a lot fewer role types than links, we store the role values once in
  /// this instance and pass references to the individual links.
  std::pair<iterator, bool> insert(const base_type&, const id_type&, const left_type&,
    const right_type&, const std::string& role = undefinedRole);

  /// Since the base type may be large, this method facilitates its insertion
  /// using move semantics. Since there are a lot fewer role types than links,
  /// we store the role values once in this instance and pass references to the
  /// individual links.
  std::pair<iterator, bool> insert(base_type&&, const id_type&, const left_type&, const right_type&,
    const std::string& role = undefinedRole);

  /// If the base_type is default-constructible, this insertion method allows
  /// you to omit the base_type instance. A new base_type will be used and the
  /// left and right types are passed to the new link using move semantics.
  /// Since there are a lot fewer role types than links, we store the role
  /// values once in this instance and pass references to the individual links.
  template <typename return_value = typename std::pair<iterator, bool> >
  typename std::enable_if<std::is_default_constructible<base_type>::value, return_value>::type
  insert(const id_type& id, const left_type& left, const right_type& right,
    const std::string& role = undefinedRole)
  {
    return insert(std::move(base_type()), id, left, right, role);
  }

  /// Check if a link with the input id exists.
  bool has(const id_type& key) const { return this->find(key) != this->end(); }

  /// Check if a link with the input value matching the tagged search criterion
  /// exists.
  template <typename tag>
  bool has(const typename LinkTraits<tag>::type&) const;

  /// Return the number of links with the input value matching the tagged search
  /// criterion.
  template <typename tag>
  std::size_t size(const typename LinkTraits<tag>::type&) const;

  /// Erase all links matching the input value for the tagged search criterion.
  template <typename tag>
  bool erase_all(const typename LinkTraits<tag>::type&);

  /// Access a link by its id and set its value associated with the tagged
  /// search criterion to a new value.
  ///
  /// NOTE: due to the indirection used for Roles to minimize storage space,
  ///       roles cannot be modified this way (and any attempts to do so will
  ///       result in a compiler error). To change a role, simply remove and
  ///       add the link with the right Role value.
  template <typename tag>
  bool set(const id_type&, const typename LinkTraits<tag>::type&);

  /// Return a set of ids corresponding to the input value for the tagged search
  /// criterion.
  template <typename tag>
  const std::set<std::reference_wrapper<const id_type> > ids(
    const typename LinkTraits<tag>::type&) const;

  /// Access the link with the input id (must be const).
  const Link& at(const id_type&) const;

  /// Access a link as its base type (can be non-const).
  LinkBase& value(const id_type&);
  const LinkBase& value(const id_type&) const;

  /// Access a tagged value associated with the input id (must be const; values
  /// can be modified using the "set" method).
  template <typename tag>
  const typename LinkTraits<tag>::type& at(const id_type&) const;

  /// Given a Left or Right tag and an associated value, return a set of the
  /// other type that links to the input value.
  template <typename tag>
  const std::set<std::reference_wrapper<const typename LinkTraits<tag>::other_type>,
    std::less<const typename LinkTraits<tag>::other_type> >
  linked_to(const typename LinkTraits<tag>::type&) const;

private:
  std::unordered_set<std::string> m_roles;
};

template <typename id_type, typename left_type, typename right_type, typename base_type>
const std::string Links<id_type, left_type, right_type, base_type>::undefinedRole = std::string("");

template <typename id_type, typename left_type, typename right_type, typename base_type>
std::pair<typename Links<id_type, left_type, right_type, base_type>::iterator, bool>
Links<id_type, left_type, right_type, base_type>::insert(const base_type& base, const id_type& id,
  const left_type& left, const right_type& right, const std::string& role)
{
  auto found = m_roles.insert(role);
  return this->insert(Link(base, id, left, right, *found.first));
}

template <typename id_type, typename left_type, typename right_type, typename base_type>
std::pair<typename Links<id_type, left_type, right_type, base_type>::iterator, bool>
Links<id_type, left_type, right_type, base_type>::insert(base_type&& base, const id_type& id,
  const left_type& left, const right_type& right, const std::string& role)
{
  auto found = m_roles.insert(role);
  return this->insert(Link(std::forward<base_type>(base), id, left, right, *found.first));
}

template <typename id_type, typename left_type, typename right_type, typename base_type>
template <typename tag>
bool Links<id_type, left_type, right_type, base_type>::has(
  const typename detail::LinkTraits<id_type, left_type, right_type, base_type, tag>::type& value)
  const
{
  auto& self = this->Parent::template get<tag>();
  auto found = self.find(value);
  return found != self.end();
}

template <typename id_type, typename left_type, typename right_type, typename base_type>
template <typename tag>
std::size_t Links<id_type, left_type, right_type, base_type>::size(
  const typename detail::LinkTraits<id_type, left_type, right_type, base_type, tag>::type& value)
  const
{
  auto& self = this->Parent::template get<tag>();
  auto range = self.equal_range(value);
  return std::distance(range.first, range.second);
}

template <typename id_type, typename left_type, typename right_type, typename base_type>
template <typename tag>
bool Links<id_type, left_type, right_type, base_type>::erase_all(
  const typename detail::LinkTraits<id_type, left_type, right_type, base_type, tag>::type& value)
{
  auto& self = this->Parent::template get<tag>();
  auto to_erase = self.equal_range(value);
  auto pos = self.erase(to_erase.first, to_erase.second);
  return pos != self.end();
}

template <typename id_type, typename left_type, typename right_type, typename base_type>
template <typename tag>
bool Links<id_type, left_type, right_type, base_type>::set(const id_type& id,
  const typename detail::LinkTraits<id_type, left_type, right_type, base_type, tag>::type& value)
{
  static_assert(!std::is_same<tag, Role>::value,
    "Roles cannot be set using the same mechanism as Left and Right values because "
    "they are cached in the Links object. To change a Role value, you must remove the "
    "link and add it back with the new Role value.");

  typedef LinkTraits<tag> traits;

  auto linkIt = this->find(id);

  if (linkIt == this->end())
  {
    throw std::out_of_range(
      "Links<id_type, left_type, right_type, base_type>::set(const id_type&, const type& value) : "
      "no link has this index");
  }

  bool modified = false;

  auto originalValue = traits::value(*linkIt);

  if (originalValue != value)
  {
    struct Modify
    {
      Modify(const typename traits::type& v)
        : value(v)
      {
      }

      void operator()(Link& link) { traits::setValue(link, value); }

      const typename traits::type& value;
    };

    auto& self = this->Parent::template get<tag>();
    auto range = self.equal_range(originalValue);
    for (auto it = range.first; it != range.second; ++it)
    {
      if (it->id == id)
      {
        modified = self.modify(it, Modify(value), Modify(originalValue));
        break;
      }
    }
    assert(modified == true);
  }
  return modified;
}

template <typename id_type, typename left_type, typename right_type, typename base_type>
template <typename tag>
const std::set<std::reference_wrapper<const id_type> >
Links<id_type, left_type, right_type, base_type>::ids(
  const typename detail::LinkTraits<id_type, left_type, right_type, base_type, tag>::type& value)
  const
{
  std::set<std::reference_wrapper<const id_type> > ids;

  auto& self = this->Parent::template get<tag>();
  auto range = self.equal_range(value);
  for (auto it = range.first; it != range.second; ++it)
  {
    ids.insert(std::cref(it->id));
  }
  return ids;
}

template <typename id_type, typename left_type, typename right_type, typename base_type>
const typename Links<id_type, left_type, right_type, base_type>::Link&
Links<id_type, left_type, right_type, base_type>::at(const id_type& id) const
{
  auto it = this->find(id);

  if (it == this->end())
  {
    throw std::out_of_range(
      "Links<id_type, left_type, right_type, base_type>::at(const id_type&) : "
      "no link has this index");
  }

  return *it;
}

template <typename id_type, typename left_type, typename right_type, typename base_type>
base_type& Links<id_type, left_type, right_type, base_type>::value(const id_type& id)
{
  return const_cast<Link&>(this->at(id));
}

template <typename id_type, typename left_type, typename right_type, typename base_type>
const base_type& Links<id_type, left_type, right_type, base_type>::value(const id_type& id) const
{
  return this->at(id);
}

template <typename id_type, typename left_type, typename right_type, typename base_type>
template <typename tag>
const typename detail::LinkTraits<id_type, left_type, right_type, base_type, tag>::type&
Links<id_type, left_type, right_type, base_type>::at(const id_type& id) const
{
  typedef LinkTraits<tag> traits;

  auto it = this->find(id);

  if (it == this->end())
  {
    throw std::out_of_range(
      "Links<id_type, left_type, right_type, base_type>::at(const id_type&) : "
      "no link has this index");
  }

  return traits::value(*it);
}

template <typename id_type, typename left_type, typename right_type, typename base_type>
template <typename tag>
const std::set<std::reference_wrapper<const typename detail::LinkTraits<id_type, left_type,
                 right_type, base_type, tag>::other_type>,
  std::less<const typename detail::LinkTraits<id_type, left_type, right_type, base_type,
    tag>::other_type> >
Links<id_type, left_type, right_type, base_type>::linked_to(
  const typename detail::LinkTraits<id_type, left_type, right_type, base_type, tag>::type& value)
  const
{
  typedef LinkTraits<tag> traits;

  std::set<std::reference_wrapper<const typename traits::other_type>,
    std::less<const typename traits::other_type> >
    values;

  auto& self = this->Parent::template get<tag>();
  auto range = self.equal_range(value);
  for (auto it = range.first; it != range.second; ++it)
  {
    values.insert(std::cref(LinkTraits<typename traits::OtherTag>::value(*it)));
  }
  return values;
}
}
}

#endif // smtk_common_Links_h