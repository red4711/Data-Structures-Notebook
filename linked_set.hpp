#ifndef LINKED_SET_HPP_
#define LINKED_SET_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include "ics_exceptions.hpp"


namespace ics {
template<class T> class LinkedSet {
  public:
    //Destructor/Constructors
    ~LinkedSet();

    LinkedSet          ();
    explicit LinkedSet (int initialLength);
    LinkedSet          (const LinkedSet<T>& to_copy);
    explicit LinkedSet (const std::initializer_list<T>& il);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit LinkedSet (const Iterable& i);


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
    LinkedSet<T>& operator = (const LinkedSet<T>& rhs);
    bool operator == (const LinkedSet<T>& rhs) const;
    bool operator != (const LinkedSet<T>& rhs) const;
    bool operator <= (const LinkedSet<T>& rhs) const;
    bool operator <  (const LinkedSet<T>& rhs) const;
    bool operator >= (const LinkedSet<T>& rhs) const;
    bool operator >  (const LinkedSet<T>& rhs) const;

    template<class T2>
    friend std::ostream& operator << (std::ostream& outs, const LinkedSet<T2>& s);



  private:
    class LN;

  public:
    class Iterator {
      public:
        //Private constructor called in begin/end, which are friends of LinkedSet<T>
        ~Iterator();
        T           erase();
        std::string str  () const;
        LinkedSet<T>::Iterator& operator ++ ();
        LinkedSet<T>::Iterator  operator ++ (int);
        bool operator == (const LinkedSet<T>::Iterator& rhs) const;
        bool operator != (const LinkedSet<T>::Iterator& rhs) const;
        T& operator *  () const;
        T* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const LinkedSet<T>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }
        friend Iterator LinkedSet<T>::begin () const;
        friend Iterator LinkedSet<T>::end   () const;

      private:
        //If can_erase is false, current indexes the "next" value (must ++ to reach it)
        LN*           current;  //if can_erase is false, this value is unusable
        LinkedSet<T>* ref_set;
        int           expected_mod_count;
        bool          can_erase = true;

        //Called in friends begin/end
        Iterator(LinkedSet<T>* iterate_over, LN* initial);
    };


    Iterator begin () const;
    Iterator end   () const;


  private:
    class LN {
      public:
        LN ()                      :value(T()) {}
        LN (const LN& ln)          : value(ln.value), next(ln.next){}
        LN (T v,  LN* n = nullptr) : value(v), next(n){}

        T   value;
        LN* next   = nullptr;
    };


    LN* front     = new LN();
    LN* trailer   = front;         //Must always point to special trailer LN
    int used      =  0;            //Cache of number of values in linked list
    int mod_count = 0;             //For sensing concurrent modification

    //Helper methods
    int  erase_at   (LN* p);
    void delete_list(LN*& front);  //Deallocate all LNs (but trailer), and set front's argument to trailer;
};





////////////////////////////////////////////////////////////////////////////////
//
//LinkedSet class and related definitions

//Destructor/Constructors

template<class T>
LinkedSet<T>::~LinkedSet() {
	delete_list(front);
	trailer = front = nullptr;
}


template<class T>
LinkedSet<T>::LinkedSet()
{
}


template<class T>
LinkedSet<T>::LinkedSet(const LinkedSet<T>& to_copy) : used(to_copy.used) {
	for (LN* to_copy_temp = to_copy.front; to_copy_temp->next; to_copy_temp = to_copy_temp->next, trailer = trailer->next){
		trailer->value = to_copy_temp->value;
		trailer->next = new LN();
	}
}


template<class T>
LinkedSet<T>::LinkedSet(const std::initializer_list<T>& il){
	for (const T& element: il)
		insert(element);
}


template<class T>
template<class Iterable>
LinkedSet<T>::LinkedSet(const Iterable& i) {
	for (auto element: i)
		insert(element);
}


////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class T>
bool LinkedSet<T>::empty() const {
	return used == 0;
}


template<class T>
int LinkedSet<T>::size() const {
	return used;
}


template<class T>
bool LinkedSet<T>::contains (const T& element) const {
	LN* traverse;
	for (traverse = front; traverse->next && traverse->value != element; traverse=traverse->next);
	return traverse != trailer;
}


template<class T>
std::string LinkedSet<T>::str() const {
	//linked_set[c->b->a->TRAILER](used=3,front=0x7424c8,trailer=0x742498,mod_count=3)
	std::ostringstream out;
	out << "linked_set[";
	for (LN* f = front; f->next; f=f->next)
		out << f->value << "->";
	out << "TRAILER](used=" << used << ",front=" << front << ",trailer=" << trailer << ",mod_count=" << mod_count << ")";
	return out.str();
}


template<class T>
template<class Iterable>
bool LinkedSet<T>::contains_all (const Iterable& i) const {
	for (auto &element: i)
		if (!contains(element)) return false;
	return true;
}


////////////////////////////////////////////////////////////////////////////////
//
//Commands


template<class T>
int LinkedSet<T>::insert(const T& element) {
	if (contains(element))
		return 0;
	trailer->value = element;
	trailer->next = new LN();
	trailer = trailer->next;
	mod_count++;
	used++;
	return 1;
}


template<class T>
int LinkedSet<T>::erase(const T& element) {
	LN* traverse;
	for (traverse = front; traverse->next && traverse->value != element; traverse=traverse->next);
	if (traverse != trailer) {
		erase_at(traverse);
		return 1;
	}
	return 0;
}


template<class T>
void LinkedSet<T>::clear() {
	delete_list(front);
	front = trailer = new LN();
}


template<class T>
template<class Iterable>
int LinkedSet<T>::insert_all(const Iterable& i) {
	int count = 0;
	for (auto &element: i)
		count += insert(element);
	return count;
}


template<class T>
template<class Iterable>
int LinkedSet<T>::erase_all(const Iterable& i) {
	int count = 0;
	for (auto &element: i)
		count += erase(element);
	return count;
}


template<class T>
template<class Iterable>
int LinkedSet<T>::retain_all(const Iterable& i) {
	int count = 0;
	LinkedSet<T> to_return;
	for (auto &element : i)
		if (contains(element))
			to_return.insert(element);
	this->LinkedSet<T>::~LinkedSet();
	new(this) LinkedSet<T>(to_return);
	return size();

}


////////////////////////////////////////////////////////////////////////////////
//
//Operators

template<class T>
LinkedSet<T>& LinkedSet<T>::operator = (const LinkedSet<T>& rhs) {
	if (this == &rhs)
		return *this;
	if (rhs.empty()){
		delete_list(front);
		front = trailer = new LN();
		return *this;
	}
	LN *temp = front, *rhs_temp = rhs.front;
	for (; rhs_temp->next && temp->next; rhs_temp=rhs_temp->next, temp=temp->next)
		temp->value = rhs_temp->value;
	if (rhs_temp->next && !temp->next)
		for (; rhs_temp->next; rhs_temp=rhs_temp->next, trailer = trailer->next){
			trailer->value = rhs_temp->value;
			trailer->next = new LN();
		}
	else{
		delete_list(temp->next);
		trailer = temp;
		trailer->next = nullptr;
	}
	used = rhs.size(), ++mod_count;
	return *this;
}


template<class T>
bool LinkedSet<T>::operator == (const LinkedSet<T>& rhs) const {
	if (this == &rhs)
		return true;
	if (used != rhs.size())
		return false;
	return contains_all(rhs);
}


template<class T>
bool LinkedSet<T>::operator != (const LinkedSet<T>& rhs) const {
	return !(*this == rhs);
}


template<class T>
bool LinkedSet<T>::operator <= (const LinkedSet<T>& rhs) const {
	if (this == &rhs)
		return true;
	if (used > rhs.size())
		return false;
	for (const auto &i : rhs)
		if (!rhs.contains(i))
		  return false;

	return true;
}


template<class T>
bool LinkedSet<T>::operator < (const LinkedSet<T>& rhs) const {
	if (this == &rhs)
	    return false;
	if (used >= rhs.size())
	    return false;
	for (const auto  &i: rhs)
	    if (!rhs.contains(i))
	      return false;

	return true;
}


template<class T>
bool LinkedSet<T>::operator >= (const LinkedSet<T>& rhs) const {
	return rhs <= *this;
}


template<class T>
bool LinkedSet<T>::operator > (const LinkedSet<T>& rhs) const {
	return rhs < *this;
}


template<class T>
std::ostream& operator << (std::ostream& outs, const LinkedSet<T>& s) {
	outs << "set[";
	if (!s.empty()){
		auto it = s.begin();
		outs << *(it++);
		for (; it != s.end(); it++)
			outs << "," << *it;
	}
	outs << "]";
	return outs;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors

template<class T>
auto LinkedSet<T>::begin () const -> LinkedSet<T>::Iterator {
	return Iterator(const_cast<LinkedSet<T>*>(this),front);
}


template<class T>
auto LinkedSet<T>::end () const -> LinkedSet<T>::Iterator {
	return Iterator(const_cast<LinkedSet<T>*>(this),trailer);
}


////////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class T>
int LinkedSet<T>::erase_at(LN* p) {
	if (p->next == trailer){
		delete trailer;
		p->next = nullptr;
		trailer = p;
	}
	else{
		LN* temp = p->next;
		p->value = temp->value;
		p->next = temp->next;
		delete temp;
	}
	++mod_count;
	--used;
	return 1;
}


template<class T>
void LinkedSet<T>::delete_list(LN*& front) {
	for (LN* temp = front; front; temp = front){
		front = front->next;
		delete temp;
	}
	mod_count++;
	used = 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//Iterator class definitions

template<class T>
LinkedSet<T>::Iterator::Iterator(LinkedSet<T>* iterate_over, LN* initial)
	:ref_set(iterate_over), current(initial), expected_mod_count(ref_set->mod_count)
{
}


template<class T>
LinkedSet<T>::Iterator::~Iterator()
{}


template<class T>
T LinkedSet<T>::Iterator::erase() {
	if (expected_mod_count != ref_set->mod_count)
		throw ConcurrentModificationError("LinkedSet::Iterator::erase");
	if (!can_erase)
		throw CannotEraseError("LinkedSet::Iterator::erase Iterator cursor already erased");
	if (!current->next)
		throw CannotEraseError("LinkedSet::Iterator::erase Iterator cursor beyond data structure");
	can_erase = false;
	T to_return = current->value;
	ref_set->erase_at(current);
	expected_mod_count = ref_set->mod_count;
	return to_return;
}


template<class T>
std::string LinkedSet<T>::Iterator::str() const {
}


template<class T>
auto LinkedSet<T>::Iterator::operator ++ () -> LinkedSet<T>::Iterator& {
	if (expected_mod_count != ref_set->mod_count)
		throw ConcurrentModificationError("LinkedSet::Iterator::operator ++");
	if (!current->next)
		return *this;
	if (can_erase)
		current = current->next;
	else
		can_erase = true;
	return *this;
}


template<class T>
auto LinkedSet<T>::Iterator::operator ++ (int) -> LinkedSet<T>::Iterator {
	if (expected_mod_count != ref_set->mod_count)
		throw ConcurrentModificationError("LinkedSet::Iterator::operator ++(int)");
	if (!current->next)
		return *this;
	Iterator to_return(*this);
	if (can_erase)
		current = current->next;
	else
		can_erase = true;
	return to_return;
}


template<class T>
bool LinkedSet<T>::Iterator::operator == (const LinkedSet<T>::Iterator& rhs) const {
	const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
	if (rhsASI == 0)
		throw IteratorTypeError("LinkedSet::Iterator::operator ==");
	if (expected_mod_count != ref_set->mod_count)
		throw ConcurrentModificationError("LinkedSet::Iterator::operator ==");
	if (ref_set != rhsASI->ref_set)
		throw ComparingDifferentIteratorsError("LinkedSet::Iterator::operator ==");
	return current == rhsASI->current;
}


template<class T>
bool LinkedSet<T>::Iterator::operator != (const LinkedSet<T>::Iterator& rhs) const {
	const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
	if (rhsASI == 0)
		throw IteratorTypeError("LinkedSet::Iterator::operator !=");
	if (expected_mod_count != ref_set->mod_count)
		throw ConcurrentModificationError("LinkedSet::Iterator::operator !=");
	if (ref_set != rhsASI->ref_set)
		throw ComparingDifferentIteratorsError("LinkedSet::Iterator::operator !=");
	return current != rhsASI->current;
}


template<class T>
T& LinkedSet<T>::Iterator::operator *() const {
	if (expected_mod_count != ref_set->mod_count)
		throw ConcurrentModificationError("LinkedSet::Iterator::operator *");
	if (!can_erase || !current) {
		std::ostringstream where;
		where << current
			  << " when front = " << ref_set->front->value << " and "
			  << " and trailer = " << ref_set->trailer;
		throw IteratorPositionIllegal("LinkedSet::Iterator::operator * Iterator illegal: "+where.str());
	}
	return current->value;
}


template<class T>
T* LinkedSet<T>::Iterator::operator ->() const {
	if (expected_mod_count != ref_set->mod_count)
		throw ConcurrentModificationError("LinkedSet::Iterator::operator *");
	if (!can_erase || !current) {
		std::ostringstream where;
		where << current
			  << " when front = " << ref_set->front->value << " and "
			  << " and trailer = " << ref_set->trailer;
		throw IteratorPositionIllegal("LinkedSet::Iterator::operator -> Iterator illegal: "+where.str());
	}
	return &current->value;
}
}

#endif /* LINKED_SET_HPP_ */
