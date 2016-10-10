// Submitter: chult(Chu, Linh)
// Partner  :
// We certify that we worked cooperatively on this programming
//   assignment, according to the rules for pair programming

#ifndef BST_MAP_HPP_
#define BST_MAP_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include "ics_exceptions.hpp"
#include "pair.hpp"
#include "array_queue.hpp"   //For traversal


namespace ics {


//Instantiate such that tlt(a,b) is true, iff a is in the left subtree rooted by b
//With a tlt specified in the template, the constructor cannot specify a clt.
//If a tlt is defaulted, then the constructor must supply a clt (they cannot both be nullptr)
template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b) = nullptr> class BSTMap {
  public:
    typedef pair<KEY,T> Entry;

    //Destructor/Constructors
    ~BSTMap();

    BSTMap          (bool (*clt)(const KEY& a, const KEY& b) = nullptr);
    BSTMap          (const BSTMap<KEY,T,tlt>& to_copy, bool (*clt)(const KEY& a, const KEY& b) = nullptr);
    explicit BSTMap (const std::initializer_list<Entry>& il, bool (*clt)(const KEY& a, const KEY& b) = nullptr);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit BSTMap (const Iterable& i, bool (*clt)(const KEY& a, const KEY& b) = nullptr);


    //Queries
    bool empty      () const;
    int  size       () const;
    bool has_key    (const KEY& key) const;
    bool has_value  (const T& value) const;
    std::string str () const; //supplies useful debugging information; contrast to operator <<


    //Commands
    T    put   (const KEY& key, const T& value);
    T    erase (const KEY& key);
    void clear ();

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    int put_all(const Iterable& i);


    //Operators

    T&       operator [] (const KEY&);
    const T& operator [] (const KEY&) const;
    BSTMap<KEY,T,tlt>& operator = (const BSTMap<KEY,T,tlt>& rhs);
    bool operator == (const BSTMap<KEY,T,tlt>& rhs) const;
    bool operator != (const BSTMap<KEY,T,tlt>& rhs) const;

    template<class KEY2,class T2, bool (*lt2)(const KEY2& a, const KEY2& b)>
    friend std::ostream& operator << (std::ostream& outs, const BSTMap<KEY2,T2,lt2>& m);



    class Iterator {
      public:
        //Private constructor called in begin/end, which are friends of BSTMap<T>
        ~Iterator();
        Entry       erase();
        std::string str  () const;
        BSTMap<KEY,T,tlt>::Iterator& operator ++ ();
        BSTMap<KEY,T,tlt>::Iterator  operator ++ (int);
        bool operator == (const BSTMap<KEY,T,tlt>::Iterator& rhs) const;
        bool operator != (const BSTMap<KEY,T,tlt>::Iterator& rhs) const;
        Entry& operator *  () const;
        Entry* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const BSTMap<KEY,T,tlt>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }
        friend Iterator BSTMap<KEY,T,tlt>::begin () const;
        friend Iterator BSTMap<KEY,T,tlt>::end   () const;

      private:
        //If can_erase is false, the value has been removed from "it" (++ does nothing)
        ArrayQueue<Entry> it;                 //Queue for all associations (from begin), to use as iterator via dequeue
        BSTMap<KEY,T,tlt>* ref_map;
        int               expected_mod_count;
        bool              can_erase = true;

        //Called in friends begin/end
        Iterator(BSTMap<KEY,T,tlt>* iterate_over, bool from_begin);
    };


    Iterator begin () const;
    Iterator end   () const;


  private:
    class TN {
      public:
        TN ()                     : left(nullptr), right(nullptr){}
        TN (const TN& tn)         : value(tn.value), left(tn.left), right(tn.right){}
        TN (Entry v, TN* l = nullptr,
                     TN* r = nullptr) : value(v), left(l), right(r){}

        Entry value;
        TN*   left;
        TN*   right;
    };

  bool (*lt) (const KEY& a, const KEY& b); // The lt used for searching BST (from template or constructor)
  TN* map       = nullptr;
  int used      = 0;                       //Cache for number of key->value pairs in the BST
  int mod_count = 0;                       //For sensing concurrent modification

  //Helper methods (find_key written iteratively, the rest recursively)
  TN*   find_key            (TN*  root, const KEY& key)                 const; //Returns reference to key's node or nullptr
  bool  has_value           (TN*  root, const T& value)                 const; //Returns whether value is is root's tree
  TN*   copy                (TN*  root)                                 const; //Copy the keys/values in root's tree (identical structure)
  void  copy_to_queue       (TN* root, ArrayQueue<Entry>& q)            const; //Fill queue with root's tree value
  bool  equals              (TN*  root, const BSTMap<KEY,T,tlt>& other) const; //Returns whether root's keys/value are all in other
  std::string string_rotated(TN* root, std::string indent)              const; //Returns string representing root's tree

  T     insert              (TN*& root, const KEY& key, const T& value);       //Put key->value, returning key's old value (or new one's, if key absent)
  T&    find_addempty       (TN*& root, const KEY& key);                       //Return reference to key's value (adding key->T() first, if key absent)
  Entry remove_closest      (TN*& root);                                       //Helper for remove
  T     remove              (TN*& root, const KEY& key);                       //Remove key->value from root's tree
  void  delete_BST          (TN*& root);                                       //Deallocate all TN in tree; root == nullptr
};




////////////////////////////////////////////////////////////////////////////////
//
//BSTMap class and related definitions

//Destructor/Constructors

template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
BSTMap<KEY,T,tlt>::~BSTMap() {
	delete_BST(map);
	map = nullptr;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
BSTMap<KEY,T,tlt>::BSTMap(bool (*clt)(const KEY& a, const KEY& b))
	:lt(tlt != nullptr ? tlt : clt)
{
	if (lt == nullptr)
		throw TemplateFunctionError("BSTMap::default constructor: neither specified");
	if (tlt != nullptr && clt != nullptr && tlt != clt)
		throw TemplateFunctionError("HeapPriorityQueue::default constructor: both specified and different");
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
BSTMap<KEY,T,tlt>::BSTMap(const BSTMap<KEY,T,tlt>& to_copy, bool (*clt)(const KEY& a, const KEY& b))
	:lt(tlt != nullptr ? tlt : clt)
{
	if (lt == nullptr)
		lt = to_copy.lt;
	if (tlt != nullptr && clt != nullptr && tlt != clt)
		throw TemplateFunctionError("BSTMap::copy constructor: both specified and different");
	lt = to_copy.lt;
	map = copy(to_copy.map);
	used = to_copy.used;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
BSTMap<KEY,T,tlt>::BSTMap(const std::initializer_list<Entry>& il, bool (*clt)(const KEY& a, const KEY& b))
	:lt(tlt != nullptr ? tlt : clt)
{
	if (lt == nullptr)
		throw TemplateFunctionError("BSTMap::initializer_list constructor: neither specified");
	if (tlt != nullptr && clt != nullptr && tlt != clt)
		throw TemplateFunctionError("BSTMap::initializer_list constructor: both specified and different");

	for (const auto &ele : il)
		put(ele.first, ele.second);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
template <class Iterable>
BSTMap<KEY,T,tlt>::BSTMap(const Iterable& i, bool (*clt)(const KEY& a, const KEY& b))
	:lt(tlt != nullptr ? tlt : clt)
{
	if (lt == nullptr)
		throw TemplateFunctionError("BSTMap::iterable constructor: neither specified");
	if (tlt != nullptr && clt != nullptr && tlt != clt)
		throw TemplateFunctionError("BSTMap::iterable constructor: both specified and different");
	for (const auto &ele : i)
		put(ele.first, ele.second);
}


////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
bool BSTMap<KEY,T,tlt>::empty() const {
	return used == 0;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
int BSTMap<KEY,T,tlt>::size() const {
	return used;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
bool BSTMap<KEY,T,tlt>::has_key (const KEY& key) const {
	return find_key(map, key);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
bool BSTMap<KEY,T,tlt>::has_value (const T& value) const {
	return has_value(map, value);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
std::string BSTMap<KEY,T,tlt>::str() const {
	std::ostringstream to_return;
	to_return << "bst_map[\n" << string_rotated(map, "") << "](used=" << used << ",mod_count=" << mod_count << ")";
	return to_return.str();

}


////////////////////////////////////////////////////////////////////////////////
//
//Commands

template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
T BSTMap<KEY,T,tlt>::put(const KEY& key, const T& value) {
	mod_count++;
	return insert(map, key, value);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
T BSTMap<KEY,T,tlt>::erase(const KEY& key) {
	T temp = remove(map, key);
	mod_count++, used--;
	return temp;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
void BSTMap<KEY,T,tlt>::clear() {
	delete_BST(map);
	map = nullptr;
	used = 0;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
template<class Iterable>
int BSTMap<KEY,T,tlt>::put_all(const Iterable& i) {
	int count = 0;
	for (const auto &ele : i)
		put(ele.first, ele.second), count++;
	return count;
}


////////////////////////////////////////////////////////////////////////////////
//
//Operators

template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
T& BSTMap<KEY,T,tlt>::operator [] (const KEY& key) {
	return find_addempty(map,key);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
const T& BSTMap<KEY,T,tlt>::operator [] (const KEY& key) const {
	return const_cast<BSTMap<KEY,T,tlt> *> (this)->find_addempty(const_cast<TN *&>(map), key);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
BSTMap<KEY,T,tlt>& BSTMap<KEY,T,tlt>::operator = (const BSTMap<KEY,T,tlt>& rhs) {
	if (this == &rhs)
		return *this;
	clear();
	lt = rhs.lt;
	map = copy(rhs.map);
	mod_count++;
	used = rhs.used;
	return *this;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
bool BSTMap<KEY,T,tlt>::operator == (const BSTMap<KEY,T,tlt>& rhs) const {
	return equals(map, rhs);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
bool BSTMap<KEY,T,tlt>::operator != (const BSTMap<KEY,T,tlt>& rhs) const {
	return !(*this == rhs);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
std::ostream& operator << (std::ostream& outs, const BSTMap<KEY,T,tlt>& m) {
	outs << "map[";
	ArrayQueue<ics::pair<KEY,T>> it;
	for (m.copy_to_queue(m.map, it); it.size(); it.dequeue()){
		ics::pair<KEY,T> temp = it.peek();
		outs << temp.first << "->" << temp.second;
		if (it.size() > 1) outs << ",";
	}
	outs << "]";
	return outs;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors

template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
auto BSTMap<KEY,T,tlt>::begin () const -> BSTMap<KEY,T,tlt>::Iterator {
	 return Iterator(const_cast<BSTMap<KEY,T,tlt>*>(this),true);
}

template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
auto BSTMap<KEY,T,tlt>::end () const -> BSTMap<KEY,T,tlt>::Iterator {
	return Iterator(const_cast<BSTMap<KEY,T,tlt>*>(this),false);
}

////////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
typename BSTMap<KEY,T,tlt>::TN* BSTMap<KEY,T,tlt>::find_key (TN* root, const KEY& key) const {
	TN *traverse = root;
	while (traverse != nullptr && traverse->value.first != key)
		traverse = lt(key, traverse->value.first) ? traverse->left : traverse->right;
	return traverse;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
bool BSTMap<KEY,T,tlt>::has_value (TN* root, const T& value) const {
	ArrayQueue<Entry> q;
	copy_to_queue(root, q);
	for (auto &i : q)
		if (i.second == value)
			return true;
	return false;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
typename BSTMap<KEY,T,tlt>::TN* BSTMap<KEY,T,tlt>::copy (TN* root) const {
	if (root == nullptr)
		return nullptr;
	return new BSTMap<KEY,T,tlt>::TN(root->value, copy(root->left), copy(root->right));
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
void BSTMap<KEY,T,tlt>::copy_to_queue (TN* root, ArrayQueue<Entry>& q) const {
	if (root == nullptr)
		return;
	copy_to_queue(root->left, q);
	q.enqueue(root->value);
	copy_to_queue(root->right, q);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
bool BSTMap<KEY,T,tlt>::equals (TN* root, const BSTMap<KEY,T,tlt>& other) const {
	if (this == &other)
		return true;
	if (used != other.size() || lt != other.lt)
		return false;
	ArrayQueue<Entry> qthis;
	for (copy_to_queue(root, qthis); qthis.size(); qthis.dequeue()){
		Entry ele = qthis.peek();
		if (!other.has_key(ele.first) || other[ele.first] != ele.second)
			return false;
	}
	return true;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
std::string BSTMap<KEY,T,tlt>::string_rotated(TN* root, std::string indent) const {
	if (root == nullptr)
		return "";
	std::ostringstream to_return;
	to_return << string_rotated(root->right, indent + "--");
	to_return << indent << root->value.first << "->" << root->value.second << "\n";
	to_return << string_rotated(root->right, indent + "--");
	return to_return.str();
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
T BSTMap<KEY,T,tlt>::insert (TN*& root, const KEY& key, const T& value) {
	if (root == nullptr){
		root = new TN(Entry(key, value));
		used++;
		return root->value.second;
	}
	if (root->value.first == key){
		T old = root->value.second;
		root->value.second = value;
		return old;
	}
	if (lt(key, root->value.first)){
		if (root->left == nullptr){
			root->left = new TN(Entry(key, value));
			used++;
			return value;
		}
	}
	else{
		if (root->right == nullptr){
			root->right = new TN(Entry(key, value));
			used++;
			return value;
		}
	}
	return insert(lt(key, root->value.first) ? root->left : root->right, key, value);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
T& BSTMap<KEY,T,tlt>::find_addempty (TN*& root, const KEY& key) {
	if (root == nullptr){
		root = new TN(Entry(key, T()));
		used++;
		mod_count++;
		return root->value.second;
	}
	if (root->value.first == key)
		return root->value.second;

	if (lt(key, root->value.first)){
		if (root->left == nullptr){
			root->left = new TN(Entry(key, T()));
			used++;
			mod_count++;
			return root->left->value.second;
		}
	}
	else{
		if (root->right == nullptr){
			root->right = new TN(Entry(key, T()));
			used++;
			mod_count++;
			return root->right->value.second;
		}
	}
	return find_addempty(lt(key, root->value.first) ? root->left : root->right, key);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
pair<KEY,T> BSTMap<KEY,T,tlt>::remove_closest(TN*& root) {
  if (root->right != nullptr)
    return remove_closest(root->right);
  else{
    Entry to_return = root->value;
    TN* to_delete = root;
    root = root->left;
    delete to_delete;
    return to_return;
  }
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
T BSTMap<KEY,T,tlt>::remove (TN*& root, const KEY& key) {
  if (root == nullptr) {
    std::ostringstream answer;
    answer << "BSTMap::erase: key(" << key << ") not in Map";
    throw KeyError(answer.str());
  }else
    if (key == root->value.first) {
      T to_return = root->value.second;
      if (root->left == nullptr) {
        TN* to_delete = root;
        root = root->right;
        delete to_delete;
      }else if (root->right == nullptr) {
        TN* to_delete = root;
        root = root->left;
        delete to_delete;
      }else
        root->value = remove_closest(root->left);
      return to_return;
    }else
      return remove( (lt(key,root->value.first) ? root->left : root->right), key);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
void BSTMap<KEY,T,tlt>::delete_BST (TN*& root) {
	if (root == nullptr)
		return;
	delete_BST(root->left), delete_BST(root->right);
	delete root;
	root = nullptr;
}






////////////////////////////////////////////////////////////////////////////////
//
//Iterator class definitions

template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
BSTMap<KEY,T,tlt>::Iterator::Iterator(BSTMap<KEY,T,tlt>* iterate_over, bool from_begin)
	:ref_map(iterate_over), expected_mod_count(ref_map->mod_count)
{
	if (from_begin)
		ref_map->copy_to_queue(ref_map->map, it);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
BSTMap<KEY,T,tlt>::Iterator::~Iterator()
{}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
auto BSTMap<KEY,T,tlt>::Iterator::erase() -> Entry {
	if (expected_mod_count != ref_map->mod_count)
		throw ConcurrentModificationError("BSTMap::Iterator::erase");
	if (!can_erase)
		throw ics::CannotEraseError("BSTMap::Iterator::erase Iterator cursor already erased");
	if (it.empty())
		throw CannotEraseError("BSTMap::Iterator::erase Iterator cursor beyond data structure");
	can_erase = false;
	Entry result = it.dequeue();
	ref_map->erase(result.first);
	expected_mod_count = ref_map->mod_count;
	return result;

}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
std::string BSTMap<KEY,T,tlt>::Iterator::str() const {
	std::ostringstream to_return;
	to_return << ref_map->str() << "(cursor=" << it.peek() << ",expected_mod_count=" << expected_mod_count << ")";
	return to_return.str();
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
auto  BSTMap<KEY,T,tlt>::Iterator::operator ++ () -> BSTMap<KEY,T,tlt>::Iterator& {
	if (expected_mod_count != ref_map->mod_count)
		throw ConcurrentModificationError("BSTMap::Iterator::operator ++");
	if (it.empty())
		return *this;
	if (can_erase)
		it.dequeue();
	else
		can_erase = true;
	return *this;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
auto BSTMap<KEY,T,tlt>::Iterator::operator ++ (int) -> BSTMap<KEY,T,tlt>::Iterator {
	if (expected_mod_count != ref_map->mod_count)
		throw ConcurrentModificationError("BSTMap::Iterator::operator ++(int)");
	if (it.empty())
		return *this;
	Iterator to_return(*this);
	if (can_erase)
		it.dequeue();
	else
		can_erase = true;
	return to_return;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
bool BSTMap<KEY,T,tlt>::Iterator::operator == (const BSTMap<KEY,T,tlt>::Iterator& rhs) const {
	const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
	if (rhsASI == 0)
		throw IteratorTypeError("BSTMap::Iterator::operator ==");
	if (expected_mod_count != ref_map->mod_count)
		throw ConcurrentModificationError("BSTMap::Iterator::operator ==");
	if (ref_map != rhsASI->ref_map)
		throw ComparingDifferentIteratorsError("BSTMap::Iterator::operator ==");
	return it == rhsASI->it;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
bool BSTMap<KEY,T,tlt>::Iterator::operator != (const BSTMap<KEY,T,tlt>::Iterator& rhs) const {
	const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
	if (rhsASI == 0)
		throw IteratorTypeError("BSTMap::Iterator::operator !=");
	if (expected_mod_count != ref_map->mod_count)
		throw ConcurrentModificationError("BSTMap::Iterator::operator !=");
	if (ref_map != rhsASI->ref_map)
		throw ComparingDifferentIteratorsError("BSTMap::Iterator::operator !=");

	return it != rhsASI->it;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
pair<KEY,T>& BSTMap<KEY,T,tlt>::Iterator::operator *() const {
	if (expected_mod_count != ref_map->mod_count)
		throw ConcurrentModificationError("HeapPriorityQueue::Iterator::operator *");
	if (!can_erase || it.empty())
		throw IteratorPositionIllegal("HeapPriorityQueue::Iterator::operator * Iterator illegal: ");
	return it.peek();
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
pair<KEY,T>* BSTMap<KEY,T,tlt>::Iterator::operator ->() const {
	if (expected_mod_count != ref_map->mod_count)
		throw ConcurrentModificationError("HeapPriorityQueue::Iterator::operator ->");
	if (!can_erase || it.empty())
		throw IteratorPositionIllegal("HeapPriorityQueue::Iterator::operator -> Iterator illegal: ");
	return &it.peek();
}


}

#endif /* BST_MAP_HPP_ */
