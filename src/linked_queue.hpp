#ifndef LINKED_QUEUE_HPP_
#define LINKED_QUEUE_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include <iterator>
#include "ics_exceptions.hpp"


namespace ics {


template<class T> class LinkedQueue {
  public:
    //Destructor/Constructors
    ~LinkedQueue();

    LinkedQueue          ();
    LinkedQueue          (const LinkedQueue<T>& to_copy);
    explicit LinkedQueue (const std::initializer_list<T>& il);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit LinkedQueue (const Iterable& i);


    //Queries
    bool empty      () const;
    int  size       () const;
    T&   peek       () const;
    std::string str () const; //supplies useful debugging information; contrast to operator <<


    //Commands
    int  enqueue (const T& element);
    T    dequeue ();
    void clear   ();

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    int enqueue_all (const Iterable& i);


    //Operators
    LinkedQueue<T>& operator = (const LinkedQueue<T>& rhs);
    bool operator == (const LinkedQueue<T>& rhs) const;
    bool operator != (const LinkedQueue<T>& rhs) const;

    template<class T2>
    friend std::ostream& operator << (std::ostream& outs, const LinkedQueue<T2>& q);



  private:
    class LN;

  public:
    class Iterator {
      public:
        //Private constructor called in begin/end, which are friends of LinkedQueue<T>
        ~Iterator();
        T           erase();
        std::string str  () const;
        LinkedQueue<T>::Iterator& operator ++ ();
        LinkedQueue<T>::Iterator  operator ++ (int);
        bool operator == (const LinkedQueue<T>::Iterator& rhs) const;
        bool operator != (const LinkedQueue<T>::Iterator& rhs) const;
        T& operator *  () const;
        T* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const LinkedQueue<T>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }
        friend Iterator LinkedQueue<T>::begin () const;
        friend Iterator LinkedQueue<T>::end   () const;

      private:
        //If can_erase is false, current indexes the "next" value (must ++ to reach it)
        LN*             prev = nullptr;  //if nullptr, current at front of list
        LN*             current;         //current == prev->next (if prev != nullptr)
        LinkedQueue<T>* ref_queue;
        int             expected_mod_count;
        bool            can_erase = true;

        //Called in friends begin/end
        Iterator(LinkedQueue<T>* iterate_over, LN* initial);
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
        LN* next = nullptr;
    };


    LN* front     =  nullptr;
    LN* rear      =  nullptr;
    int used      =  0;            //Cache for number of values in linked list
    int mod_count =  0;            //For sensing any concurrent modifications

    //Helper methods
    void delete_list(LN*& front);  //Deallocate all LNs, and set front's argument to nullptr;
};





////////////////////////////////////////////////////////////////////////////////
//
//LinkedQueue class and related definitions

//Destructor/Constructors

template<class T>
LinkedQueue<T>::~LinkedQueue(){
	delete_list(front);
}


template<class T>
LinkedQueue<T>::LinkedQueue()
	:front(nullptr), rear(nullptr){
}


template<class T>
LinkedQueue<T>::LinkedQueue(const LinkedQueue<T>& to_copy)
{
	LN *_temp = to_copy.front;
	for (front = rear = new LN(_temp->value, nullptr); _temp->next; _temp=_temp->next)
		enqueue(_temp->next->value);
	used++;
}


template<class T>
LinkedQueue<T>::LinkedQueue(const std::initializer_list<T>& il)
{
	if (il.size() > 0){
		const T* it = il.begin();
		front = rear = new LN(*(it++));
		for (; it != il.end(); ++it)
			enqueue(*it);
		used++;
	}
}


template<class T>
template<class Iterable>
LinkedQueue<T>::LinkedQueue(const Iterable& i) {
	used = enqueue_all(i);
}


////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class T>
bool LinkedQueue<T>::empty() const {
	return used == 0;
}


template<class T>
int LinkedQueue<T>::size() const {
	return used;
}


template<class T>
T& LinkedQueue<T>::peek () const {
	if (this->empty())
	    throw EmptyError("ArrayQueue::peek");
	return front->value;
}


template<class T>
std::string LinkedQueue<T>::str() const {
	std::ostringstream result;
	result << "linked_queue[";
	if(!empty()){
		result << front->value;
		for (LN* it = front->next; it; it=it->next)
			result << "->" << it->value;
	}
	result << "](used=" << used << ",front=" << front <<
					",rear=" << rear << ",mod_count=" << mod_count << ")";
	return result.str();
}


////////////////////////////////////////////////////////////////////////////////
//
//Commands

template<class T>
int LinkedQueue<T>::enqueue(const T& element) {
	rear = (front ? rear->next : front) = new LN(element);
	++mod_count, ++used;
	return 1;
}


template<class T>
T LinkedQueue<T>::dequeue() {
	LN* temp = front;
	T result = temp->value;
	front = front->next;
	delete temp;
	--used, ++mod_count;
	return result;
}


template<class T>
void LinkedQueue<T>::clear() {
	delete_list(front);
	++mod_count;
}


template<class T>
template<class Iterable>
int LinkedQueue<T>::enqueue_all(const Iterable& i) {
	int count = 0;
	for (auto &it: i)
		enqueue(it), ++count;
	return count;
}


////////////////////////////////////////////////////////////////////////////////
//
//Operators

template<class T>
LinkedQueue<T>& LinkedQueue<T>::operator = (const LinkedQueue<T>& rhs) {
	if (this == &rhs)
		return *this;
	if (rhs.empty()) {
		delete_list(front);
		return *this;
	}
	if (this->empty()){
		front = rear = new LN(rhs.front->value);
		for (LN *rhs_temp = rhs.front->next; rhs_temp; rhs_temp=rhs_temp->next, rear = rear->next)
			rear->next = new LN(rhs_temp->value);
	}
	else {
		for (LN* temp = front, *rhs_temp = rhs.front; rhs_temp;rhs_temp = rhs_temp->next){
			temp->value = rhs_temp->value;
			if (temp->next)
				rear = temp = temp->next;
			else if (rhs_temp->next)
				rear = temp = temp->next = new LN();
		}
		delete_list(rear->next);
	}
	used = rhs.size(), ++mod_count;
	return *this;
}


template<class T>
bool LinkedQueue<T>::operator == (const LinkedQueue<T>& rhs) const {
	if (used != rhs.used) return 0;
	for (LN *this_it = front, *rhs_it = rhs.front; this_it && rhs_it; this_it = this_it->next, rhs_it = rhs_it->next){
		if (this_it->value != rhs_it->value) return 0;
	}
	return 1;
}


template<class T>
bool LinkedQueue<T>::operator != (const LinkedQueue<T>& rhs) const {
	return !(*this == rhs);
}


template<class T>
std::ostream& operator << (std::ostream& outs, const LinkedQueue<T>& q) {
	outs << "queue[";
	if (!q.empty()){
		auto it = q.begin();
		outs << *(it++);
		for (; it != q.end(); it++)
			outs << "," << *it;
	}
	outs << "]:rear";
	return outs;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors

template<class T>
auto LinkedQueue<T>::begin () const -> LinkedQueue<T>::Iterator {
	return Iterator(const_cast<LinkedQueue<T>*>(this),front);
}

template<class T>
auto LinkedQueue<T>::end () const -> LinkedQueue<T>::Iterator {
	return Iterator(const_cast<LinkedQueue<T>*>(this),nullptr);
}


////////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class T>
void LinkedQueue<T>::delete_list(LN*& front) {
	for (LN* temp = front; front; temp = front){
		front = front->next;
		delete temp;
	}
	used = 0;
	rear = nullptr;
}





////////////////////////////////////////////////////////////////////////////////
//
//Iterator class definitions

template<class T>
LinkedQueue<T>::Iterator::Iterator(LinkedQueue<T>* iterate_over, LN* initial)
	:current(initial), ref_queue(iterate_over), expected_mod_count(ref_queue->mod_count)
{
}


template<class T>
LinkedQueue<T>::Iterator::~Iterator()
{}


template<class T>
T LinkedQueue<T>::Iterator::erase() {
	if (expected_mod_count != ref_queue->mod_count)
		throw ConcurrentModificationError("ArrayQueue::Iterator::erase");
	if (!can_erase)
	    throw CannotEraseError("ArrayQueue::Iterator::erase Iterator cursor already erased");
	can_erase = false;

	T to_return = current->value;
	if (!prev){
		current = current->next;
		ref_queue->dequeue();
	}
	else{
		prev->next = current->next;
		if (current == ref_queue->rear) ref_queue->rear = prev;
		delete current;
		current = prev->next;
		ref_queue->used--;
		ref_queue->mod_count++;
	}
	expected_mod_count = ref_queue->mod_count;
	return to_return;
}


template<class T>
std::string LinkedQueue<T>::Iterator::str() const {
	std::ostringstream answer;
	answer << ref_queue->str() << "(current=" << current->value << ",previous=" << prev->value << ",expected_mod_count=" << expected_mod_count << ",can_erase=" << can_erase << ")";
	return answer.str();
}


template<class T>
auto LinkedQueue<T>::Iterator::operator ++ () -> LinkedQueue<T>::Iterator& {
	if (expected_mod_count != ref_queue->mod_count)
	    throw ConcurrentModificationError("LinkedQueue::Iterator::operator ++");
	if (!current)
		can_erase = false;
	else if (can_erase)
		current = (prev = current)->next;
	else
		can_erase = true;
	return *this;
}


template<class T>
auto LinkedQueue<T>::Iterator::operator ++ (int) -> LinkedQueue<T>::Iterator {
	if (expected_mod_count != ref_queue->mod_count)
		throw ConcurrentModificationError("LinkedQueue::Iterator::operator ++");
	Iterator to_return(*this);
	if (!current)
		can_erase = false;
	else if (can_erase)
		current = (prev = current)->next;
	else
		can_erase = true;
	return to_return;
}


template<class T>
bool LinkedQueue<T>::Iterator::operator == (const LinkedQueue<T>::Iterator& rhs) const {
	const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
	if (rhsASI == 0)
		throw IteratorTypeError("LinkedQueue::Iterator::operator ==");
	if (expected_mod_count != ref_queue->mod_count)
		throw ConcurrentModificationError("LinkedQueue::Iterator::operator ==");
	if (ref_queue != rhsASI->ref_queue)
		throw ComparingDifferentIteratorsError("LinkedQueue::Iterator::operator ==");
	return current == rhsASI->current;
}

template<class T>
bool LinkedQueue<T>::Iterator::operator != (const LinkedQueue<T>::Iterator& rhs) const {
	const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
	if (rhsASI == 0)
		throw IteratorTypeError("LinkedQueue::Iterator::operator !=");
	if (expected_mod_count != ref_queue->mod_count)
		throw ConcurrentModificationError("LinkedQueue::Iterator::operator !=");
	if (ref_queue != rhsASI->ref_queue)
		throw ComparingDifferentIteratorsError("LinkedQueue::Iterator::operator !=");
	return current != rhsASI->current;
}


template<class T>
T& LinkedQueue<T>::Iterator::operator *() const {
	if (expected_mod_count != ref_queue->mod_count)
		throw ConcurrentModificationError("LinkedQueue::Iterator::operator *");
	if (!can_erase || !current) {
		std::ostringstream where;
		where << current
			  << " when front = " << ref_queue->front->value << " and "
			  << " and rear = " << ref_queue->rear->value;
		throw IteratorPositionIllegal("LinkedQueue::Iterator::operator * Iterator illegal: "+where.str());
	}
	return current->value;
}


template<class T>
T* LinkedQueue<T>::Iterator::operator ->() const {
	if (expected_mod_count != ref_queue->mod_count)
		throw ConcurrentModificationError("LinkedQueue::Iterator::operator ->");
	if (!can_erase || !current) {
		std::ostringstream where;
		where << current
			  << " when front = " << ref_queue->front->value << " and "
			  << " and rear = " << ref_queue->rear->value;
		throw IteratorPositionIllegal("LinkedQueue::Iterator::operator -> Iterator illegal: "+where.str());
	}
	return &current->value;
}


}

#endif /* LINKED_QUEUE_HPP_ */
