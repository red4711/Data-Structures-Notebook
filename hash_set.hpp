#ifndef HASH_SET_HPP_
#define HASH_SET_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include "ics_exceptions.hpp"
#include "pair.hpp"


namespace ics {


//Instantiate the templated class supplying thash(a): produces a hash value for a.
//If thash is defaulted to nullptr in the template, then a constructor must supply chash.
//If both thash and chash are supplied, then they must be the same (by ==) function.
//If neither is supplied, or both are supplied but different, TemplateFunctionError is raised.
//The (unique) non-nullptr value supplied by thash/chash is stored in the instance variable hash.
template<class T, int (*thash)(const T& a) = nullptr> class HashSet {
  public:
    //Destructor/Constructors
    ~HashSet ();

    HashSet (double the_load_threshold = 1.0, int (*chash)(const T& a) = nullptr);
    explicit HashSet (int initial_bins, double the_load_threshold = 1.0, int (*chash)(const T& k) = nullptr);
    HashSet (const HashSet<T,thash>& to_copy, double the_load_threshold = 1.0, int (*chash)(const T& a) = nullptr);
    explicit HashSet (const std::initializer_list<T>& il, double the_load_threshold = 1.0, int (*chash)(const T& a) = nullptr);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit HashSet (const Iterable& i, double the_load_threshold = 1.0, int (*chash)(const T& a) = nullptr);


    //Queries
    bool empty      () const;
    int  size       () const;
    bool contains   (const T& element) const;
    std::string str () const; //supplies useful debugging information; contrast to operator <<

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    bool contains_all (const Iterable& i) const;


    //Commands
    int  insert (const T& element);
    int  erase  (const T& element);
    void clear  ();

    //Iterable class must support "for" loop: .begin()/.end() and prefix ++ on returned result

    template <class Iterable>
    int insert_all(const Iterable& i);

    template <class Iterable>
    int erase_all(const Iterable& i);

    template<class Iterable>
    int retain_all(const Iterable& i);


    //Operators
    HashSet<T,thash>& operator = (const HashSet<T,thash>& rhs);
    bool operator == (const HashSet<T,thash>& rhs) const;
    bool operator != (const HashSet<T,thash>& rhs) const;
    bool operator <= (const HashSet<T,thash>& rhs) const;
    bool operator <  (const HashSet<T,thash>& rhs) const;
    bool operator >= (const HashSet<T,thash>& rhs) const;
    bool operator >  (const HashSet<T,thash>& rhs) const;

    template<class T2, int (hash2) (const T2& a)>
    friend std::ostream& operator << (std::ostream& outs, const HashSet<T2,hash2>& s);



  private:
    class LN;

  public:
    class Iterator {
      public:
        typedef pair<int,LN*> Cursor;

        //Private constructor called in begin/end, which are friends of HashSet<T,thash>
        ~Iterator();
        T           erase();
        std::string str  () const;
        HashSet<T,thash>::Iterator& operator ++ ();
        HashSet<T,thash>::Iterator  operator ++ (int);
        bool operator == (const HashSet<T,thash>::Iterator& rhs) const;
        bool operator != (const HashSet<T,thash>::Iterator& rhs) const;
        T& operator *  () const;
        T* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const HashSet<T,thash>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }
        friend Iterator HashSet<T,thash>::begin () const;
        friend Iterator HashSet<T,thash>::end   () const;

      private:
        //If can_erase is false, current indexes the "next" value (must ++ to reach it)
        Cursor              current; //Bin Index and Cursor; stop: LN* == nullptr
        HashSet<T,thash>*   ref_set;
        int                 expected_mod_count;
        bool                can_erase = true;

        //Helper methods
        void advance_cursors();

        //Called in friends begin/end
        Iterator(HashSet<T,thash>* iterate_over, bool from_begin);
    };


    Iterator begin () const;
    Iterator end   () const;


  private:
    class LN {
      public:
        LN ()                      {}
        LN (const LN& ln)          : value(ln.value), next(ln.next){}
        LN (T v,  LN* n = nullptr) : value(v), next(n){}

        T   value;
        LN* next   = nullptr;
    };

public:
  int (*hash)(const T& k);   //Hashing function used (from template or constructor)
private:
  LN** set      = nullptr;   //Pointer to array of pointers: each bin stores a list with a trailer node
  double load_threshold;     //used/bins <= load_threshold
  int bins      = 1;         //# bins in array (should start at 1 so hash_compress doesn't % 0)
  int used      = 0;         //Cache for number of key->value pairs in the hash table
  int mod_count = 0;         //For sensing concurrent modification


  //Helper methods
  int   hash_compress        (const T& key)              const;  //hash function ranged to [0,bins-1]
  LN*   find_element         (int bin, const T& element) const;  //Returns reference to element's node or nullptr
  LN*   copy_list            (LN*   l)                   const;  //Copy the elements in a bin (order irrelevant)
  LN**  copy_hash_table      (LN** ht, int bins)         const;  //Copy the bins/keys/values in ht tree (order in bins irrelevant)

  void  ensure_load_threshold(int new_used);                     //Reallocate if load_threshold > load_threshold
  void  delete_hash_table    (LN**& ht, int bins);               //Deallocate all LN in ht (and the ht itself; ht == nullptr)
};





//HashSet class and related definitions

////////////////////////////////////////////////////////////////////////////////
//
//Destructor/Constructors

template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::~HashSet() {
  delete_hash_table(set,bins);
}


template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::HashSet(double the_load_threshold, int (*chash)(const T& element))
: hash(thash != nullptr ? thash : chash), load_threshold(the_load_threshold) {
  if (hash == nullptr)
    throw TemplateFunctionError("HashSet::default constructor: neither specified");
  if (thash != nullptr && chash != nullptr && thash != chash)
    throw TemplateFunctionError("HashSet::default constructor: both specified and different");

  set = new LN*[bins];
  for (int b=0; b<bins; ++b)
    set[b] = new LN();
}


template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::HashSet(int initial_bins, double the_load_threshold, int (*chash)(const T& element))
: hash(thash != nullptr ? thash : chash), bins(initial_bins), load_threshold(the_load_threshold) {
  if (hash == nullptr)
    throw TemplateFunctionError("HashSet::length constructor: neither specified");
  if (thash != nullptr && chash != nullptr && thash != chash)
    throw TemplateFunctionError("HashSet::length constructor: both specified and different");

  if (bins < 1)
    bins = 1;
  set = new LN*[bins];
  for (int b=0; b<bins; ++b)
    set[b] = new LN();
}


template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::HashSet(const HashSet<T,thash>& to_copy, double the_load_threshold, int (*chash)(const T& element))
: hash(thash != nullptr ? thash : chash), load_threshold(the_load_threshold), bins(to_copy.bins) {
  if (hash == nullptr)
    hash = to_copy.hash;//throw TemplateFunctionError("HashSet::copy constructor: neither specified");
  if (thash != nullptr && chash != nullptr && thash != chash)
    throw TemplateFunctionError("HashSet::copy constructor: both specified and different");

  if (hash == to_copy.hash && (double)to_copy.size()/to_copy.bins <= the_load_threshold) {
    used = to_copy.used;
    set  = copy_hash_table(to_copy.set,to_copy.bins);
  }else {
    bins = std::max(1,int(to_copy.size()/load_threshold));
    set = new LN*[bins];
    for (int b=0; b<bins; ++b)
      set[b] = new LN();         //Put a trailer node in bin

    for (int b=0; b<to_copy.bins; ++b)
      for (LN* c = to_copy.set[b]; c->next!=nullptr; c=c->next)
        insert(c->value);
  }
}


template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::HashSet(const std::initializer_list<T>& il, double the_load_threshold, int (*chash)(const T& element))
: hash(thash != nullptr ? thash : chash), load_threshold(the_load_threshold), bins(std::max(1,int(il.size()/the_load_threshold))) {
  if (hash == nullptr)
    throw TemplateFunctionError("HashSet::initializer_list constructor: neither specified");
  if (thash != nullptr && chash != nullptr && thash != chash)
    throw TemplateFunctionError("HashSet::initializer_list constructor: both specified and different");

  set = new LN*[bins];
  for (int b=0; b<bins; ++b)
    set[b] = new LN();

  for (const T& v : il)
    insert(v);
}


template<class T, int (*thash)(const T& a)>
template<class Iterable>
HashSet<T,thash>::HashSet(const Iterable& i, double the_load_threshold, int (*chash)(const T& a))
: hash(thash != nullptr ? thash : chash), load_threshold(the_load_threshold), bins(std::max(1,int(i.size()/the_load_threshold))) {
  if (hash == nullptr)
    throw TemplateFunctionError("HashSet::Iterable constructor: neither specified");
  if (thash != nullptr && chash != nullptr && thash != chash)
    throw TemplateFunctionError("HashSet::Iterable constructor: both specified and different");

  set = new LN*[bins];
  for (int b=0; b<bins; ++b)
    set[b] = new LN();

  for (const T& v : i)
    insert(v);
}


////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::empty() const {
  return used == 0;
}


template<class T, int (*thash)(const T& a)>
int HashSet<T,thash>::size() const {
  return used;
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::contains (const T& element) const {
  return find_element(hash_compress(element),element) != nullptr;
}


template<class T, int (*thash)(const T& a)>
std::string HashSet<T,thash>::str() const {
  std::ostringstream answer;
  answer << "HashSet[";
  if (bins != 0) {
    answer << std::endl;
    for (int b=0; b<bins; ++b) {
      answer << "bin[" << b << "] = ";
      for (LN* c = set[b]; c->next!=nullptr; c=c->next)
        answer << c->value << " -> " ;
      answer << "TRAILER" << std::endl;
    }
  }

  answer  << "(load_threshold=" << load_threshold << ",bins=" << bins << ",used=" <<used <<",mod_count=" << mod_count << ")";
  return answer.str();
}


template<class T, int (*thash)(const T& a)>
template <class Iterable>
bool HashSet<T,thash>::contains_all(const Iterable& i) const {
  for (const T& v : i)
    if (!contains(v))
      return false;

  return true;
}


////////////////////////////////////////////////////////////////////////////////
//
//Commands

template<class T, int (*thash)(const T& a)>
int HashSet<T,thash>::insert(const T& element) {
  int bin = hash_compress(element);
  LN* c = find_element(bin,element);
  if (c != nullptr)
      return 0;

  ensure_load_threshold(used+1);

  ++used;
  ++mod_count;
  bin = hash_compress(element);         //bins may have changed in ensure_load_threshold!
  set[bin] = new LN(element,set[bin]);  //easy to put at front: bin LNs unordered
  return 1;
}


template<class T, int (*thash)(const T& a)>
int HashSet<T,thash>::erase(const T& element) {
  LN* c = find_element(hash_compress(element),element);
  if (c == nullptr)
    return 0;

  LN* to_delete = c->next;
  *c = *(c->next);
  delete to_delete;
  --used;
  ++mod_count;
  return 1;
}


template<class T, int (*thash)(const T& a)>
void HashSet<T,thash>::clear() {
  for (int b=0; b<bins; ++b) {
    LN* l=set[b];
    for (; l->next!=nullptr; /*See body*/) {
      LN* to_delete = l;
      l = l->next;
      delete to_delete;
    }
    set[b] = l;
  }

  used = 0;
  ++mod_count;
}


template<class T, int (*thash)(const T& a)>
template<class Iterable>
int HashSet<T,thash>::insert_all(const Iterable& i) {
  int count = 0;
  for (const T& v : i)
    count += insert(v);

  return count;
}


template<class T, int (*thash)(const T& a)>
template<class Iterable>
int HashSet<T,thash>::erase_all(const Iterable& i) {
  int count = 0;
  for (const T& v : i)
    count += erase(v);
  return count;
}


template<class T, int (*thash)(const T& a)>
template<class Iterable>
int HashSet<T,thash>::retain_all(const Iterable& i) {
  HashSet<T,thash> s(i);

  int count = 0;
  for (int b=0; b<bins; ++b)
    for (LN* c=set[b]; c->next!=nullptr; /*See body*/) {
      if (s.contains(c->value))
        c = c-> next;
      else{
        LN* to_delete = c->next;
        *c = *(c->next);
        delete to_delete;
        ++count;
      }
    }

  used -= count;
  return count;
}


////////////////////////////////////////////////////////////////////////////////
//
//Operators

template<class T, int (*thash)(const T& a)>
HashSet<T,thash>& HashSet<T,thash>::operator = (const HashSet<T,thash>& rhs) {
  if (this == &rhs)
    return *this;

  if (hash == rhs.hash && (double)rhs.size()/rhs.bins <= load_threshold) {
    delete_hash_table(set,bins);
    set  = copy_hash_table(rhs.set,rhs.bins);
    bins = rhs.bins;
    used = rhs.used;
  }else{
    clear();
    for (int b=0; b<rhs.bins; ++b)
      for (LN* c = rhs.set[b]; c->next!=nullptr; c=c->next)
        insert(c->value);
  }

  ++mod_count;
  return *this;
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::operator == (const HashSet<T,thash>& rhs) const {
  if (this == &rhs)
    return true;
  if (used != rhs.size())
    return false;

  for (int b=0; b<bins; ++b)
    for (LN* c=set[b]; c->next!=nullptr; c=c->next)
       if (!rhs.contains(c->value))
         return false;

  return true;
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::operator != (const HashSet<T,thash>& rhs) const {
  return !(*this == rhs);
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::operator <= (const HashSet<T,thash>& rhs) const {
  if (this == &rhs)
    return true;
  if (used > rhs.size())
    return false;

  for (int b=0; b<bins; ++b)
    for (LN* c=set[b]; c->next!=nullptr; c=c->next)
      if (!rhs.contains(c->value))
        return false;

  return true;
}

template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::operator < (const HashSet<T,thash>& rhs) const {
  if (this == &rhs)
    return false;
  if (used >= rhs.size())
    return false;

  for (int b=0; b<bins; ++b)
    for (LN* c=set[b]; c->next!=nullptr; c=c->next)
      if (!rhs.contains(c->value))
        return false;

  return true;
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::operator >= (const HashSet<T,thash>& rhs) const {
  return rhs <= *this;
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::operator > (const HashSet<T,thash>& rhs) const {
  return rhs < *this;
}


template<class T, int (*thash)(const T& a)>
std::ostream& operator << (std::ostream& outs, const HashSet<T,thash>& s) {
  outs  << "set[";

  int printed = 0;
  for (int b=0; b<s.bins; ++b)
    for (typename HashSet<T,thash>::LN* c = s.set[b]; c->next != nullptr; c = c->next)
      outs << (printed++ == 0? "" : ",") << c->value;

  outs << "]";
  return outs;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors

template<class T, int (*thash)(const T& a)>
auto HashSet<T,thash>::begin () const -> HashSet<T,thash>::Iterator {
  return Iterator(const_cast<HashSet<T,thash>*>(this),true);
}


template<class T, int (*thash)(const T& a)>
auto HashSet<T,thash>::end () const -> HashSet<T,thash>::Iterator {
  return Iterator(const_cast<HashSet<T,thash>*>(this),false);
}


////////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class T, int (*thash)(const T& a)>
int HashSet<T,thash>::hash_compress (const T& element) const {
  return abs(hash(element)) % bins;
}


template<class T, int (*thash)(const T& a)>
typename HashSet<T,thash>::LN* HashSet<T,thash>::find_element (int bin, const T& element) const {
  for (LN* c = set[bin]; c->next!=nullptr; c=c->next)
    if (element == c->value)
      return c;

  return nullptr;
}

template<class T, int (*thash)(const T& a)>
typename HashSet<T,thash>::LN* HashSet<T,thash>::copy_list (LN* l) const {
//    //Recursive
//    if (l == nullptr)
//      return nullptr;
//    else
//      return new LN(l->value, copy_list(l->next));

  //Iterative: order in bin makes no difference, but Trailer must be at end
  if (l->next == nullptr)
    return new LN();

   LN* answer = new LN(l->value,new LN());
   for (LN* c = l->next; c->next != nullptr; c = c->next)
     answer = new LN(c->value,answer);

  return answer;
}


template<class T, int (*thash)(const T& a)>
typename HashSet<T,thash>::LN** HashSet<T,thash>::copy_hash_table (LN** ht, int bins) const {
  LN** answer = new LN*[bins];
  for (int b=0; b<bins; ++b)
     answer[b] = copy_list(ht[b]);
  return answer;
}


template<class T, int (*thash)(const T& a)>
void HashSet<T,thash>::ensure_load_threshold(int new_used) {
  if (double(new_used)/double(bins) <= load_threshold)
    return;

  LN** old_set  = set;
  int  old_bins = bins;

  bins = 2*old_bins;
  set = new LN*[bins];

  for (int b=0; b<bins; ++b)
    set[b] = new LN();

  for (int b=0; b<old_bins; ++b) {
    LN* c=old_set[b];
    for (; c->next!=nullptr; /*See body*/) {
      int bin = hash_compress(c->value);
      LN* to_move = c;
      c = c->next;
      to_move->next = set[bin];
      set[bin] = to_move;
    }
    delete c;           //deallocate trailers in old_map
  }
  delete [] old_set;
}


template<class T, int (*thash)(const T& a)>
void HashSet<T,thash>::delete_hash_table (LN**& ht, int bins) {
  for (int b=0; b<bins; ++b)
    for (LN* c=ht[b]; c!=nullptr; /*See body*/) {
      LN* to_delete = c;
      c = c->next;
      delete to_delete;
  }
  delete[] ht;
  ht = nullptr;
}






////////////////////////////////////////////////////////////////////////////////
//
//Iterator class definitions

template<class T, int (*thash)(const T& a)>
void HashSet<T,thash>::Iterator::advance_cursors() {
  if (current.second != nullptr && current.second->next != nullptr && current.second->next->next != nullptr) {
    current.second = current.second->next;
    return;
  }else
    for (int b=current.first+1; b<ref_set->bins; ++b)
      if (ref_set->set[b]->next != nullptr) {
        current.first  = b;
        current.second = ref_set->set[b];
        return;
      }

  //Not found
  current.first  = -1;
  current.second = nullptr;
}


template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::Iterator::Iterator(HashSet<T,thash>* iterate_over, bool begin)
: ref_set(iterate_over) {
  current = Cursor(-1,nullptr);
  if (begin)
     advance_cursors();
  expected_mod_count = ref_set->mod_count;
}


template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::Iterator::~Iterator()
{}


template<class T, int (*thash)(const T& a)>
T HashSet<T,thash>::Iterator::erase() {
  if (expected_mod_count != ref_set->mod_count)
    throw ConcurrentModificationError("HashSet::Iterator::erase");
  if (!can_erase)
    throw CannotEraseError("HashSet::Iterator::erase Iterator cursor already erased");
  if (current.second == nullptr)
    throw CannotEraseError("HashSet::Iterator::erase Iterator cursor beyond data structure");

  can_erase = false;
  T to_return = current.second->value;
  LN* to_delete = current.second->next;

  *current.second = *(current.second->next);
  --ref_set->used;
  ++ref_set->mod_count;
  expected_mod_count = ref_set->mod_count;
  delete to_delete;

  return to_return;
}


template<class T, int (*thash)(const T& a)>
std::string HashSet<T,thash>::Iterator::str() const {
  std::ostringstream answer;
  answer << ref_set->str() << "(current=" << current.first << "/" << current.second << ",expected_mod_count=" << expected_mod_count << ",can_erase=" << can_erase << ")";
  return answer.str();
}


template<class T, int (*thash)(const T& a)>
auto  HashSet<T,thash>::Iterator::operator ++ () -> HashSet<T,thash>::Iterator& {
  if (expected_mod_count != ref_set->mod_count)
    throw ConcurrentModificationError("HashSet::Iterator::operator ++");

  if (current.second == nullptr)
    return *this;

  if (can_erase || current.second->next == nullptr)
    advance_cursors();

  can_erase = true;
  return *this;
}


template<class T, int (*thash)(const T& a)>
auto  HashSet<T,thash>::Iterator::operator ++ (int) -> HashSet<T,thash>::Iterator {
  if (expected_mod_count != ref_set->mod_count)
    throw ConcurrentModificationError("HashSet::Iterator::operator ++(int)");

  Iterator* to_return = new Iterator(*this);
  if (current.second == nullptr)
    return *to_return;

  if (can_erase || current.second->next == nullptr)
    advance_cursors();

  can_erase = true;
  return *to_return;
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::Iterator::operator == (const HashSet<T,thash>::Iterator& rhs) const {
  const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
  if (rhsASI == 0)
    throw IteratorTypeError("HashSet::Iterator::operator ==");
  if (expected_mod_count != ref_set->mod_count)
    throw ConcurrentModificationError("HashSet::Iterator::operator ==");
  if (ref_set != rhsASI->ref_set)
    throw ComparingDifferentIteratorsError("HashSet::Iterator::operator ==");

  return this->current.second == rhsASI->current.second;
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::Iterator::operator != (const HashSet<T,thash>::Iterator& rhs) const {
  const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
  if (rhsASI == 0)
    throw IteratorTypeError("HashSet::Iterator::operator !=");
  if (expected_mod_count != ref_set->mod_count)
    throw ConcurrentModificationError("HashSet::Iterator::operator !=");
  if (ref_set != rhsASI->ref_set)
    throw ComparingDifferentIteratorsError("HashSet::Iterator::operator !=");

  return this->current.second != rhsASI->current.second;
}

template<class T, int (*thash)(const T& a)>
T& HashSet<T,thash>::Iterator::operator *() const {
  if (expected_mod_count !=
      ref_set->mod_count)
    throw ConcurrentModificationError("HashSet::Iterator::operator *");
  if (!can_erase || current.second == nullptr)
    throw IteratorPositionIllegal("HashSet::Iterator::operator * Iterator illegal: exhausted");

  return current.second->value;
}

template<class T, int (*thash)(const T& a)>
T* HashSet<T,thash>::Iterator::operator ->() const {
  if (expected_mod_count !=
      ref_set->mod_count)
    throw ConcurrentModificationError("HashSet::Iterator::operator *");
  if (!can_erase || current.second == nullptr)
    throw IteratorPositionIllegal("HashSet::Iterator::operator * Iterator illegal: exhausted");

  return &(current.second->value);
}

}

#endif /* HASH_SET_HPP_ */
