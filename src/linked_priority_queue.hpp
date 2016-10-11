#ifndef LINKED_PRIORITY_QUEUE_HPP_
#define LINKED_PRIORITY_QUEUE_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include "ics_exceptions.hpp"
#include "array_stack.hpp"      //See operator <<


namespace ics {

//Instantiate the template such that tgt(a,b) is true, iff a has higher priority than b
//With a tgt specified in the template, the constructor cannot specify a cgt.
//If a tgt is defaulted, then the constructor must supply a cgt (they cannot both be nullptr)
template<class T, bool (*tgt)(const T& a, const T& b) = nullptr> class LinkedPriorityQueue {
  public:
    //Destructor/Constructors
    ~LinkedPriorityQueue();

    LinkedPriorityQueue          (bool (*cgt)(const T& a, const T& b) = nullptr);
    LinkedPriorityQueue          (const LinkedPriorityQueue<T,tgt>& to_copy, bool (*cgt)(const T& a, const T& b) = nullptr);
    explicit LinkedPriorityQueue (const std::initializer_list<T>& il, bool (*cgt)(const T& a, const T& b) = nullptr);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit LinkedPriorityQueue (const Iterable& i, bool (*cgt)(const T& a, const T& b) = nullptr);


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
    LinkedPriorityQueue<T,tgt>& operator = (const LinkedPriorityQueue<T,tgt>& rhs);
    bool operator == (const LinkedPriorityQueue<T,tgt>& rhs) const;
    bool operator != (const LinkedPriorityQueue<T,tgt>& rhs) const;

    template<class T2, bool (*gt2)(const T2& a, const T2& b)>
    friend std::ostream& operator << (std::ostream& outs, const LinkedPriorityQueue<T2,gt2>& pq);



  private:
    class LN;

  public:
    class Iterator {
      public:
        //Private constructor called in begin/end, which are friends of LinkedPriorityQueue<T,tgt>
        ~Iterator();
        T           erase();
        std::string str  () const;
        LinkedPriorityQueue<T,tgt>::Iterator& operator ++ ();
        LinkedPriorityQueue<T,tgt>::Iterator  operator ++ (int);
        bool operator == (const LinkedPriorityQueue<T,tgt>::Iterator& rhs) const;
        bool operator != (const LinkedPriorityQueue<T,tgt>::Iterator& rhs) const;
        T& operator *  () const;
        T* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const LinkedPriorityQueue<T,tgt>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }
        friend Iterator LinkedPriorityQueue<T,tgt>::begin () const;
        friend Iterator LinkedPriorityQueue<T,tgt>::end   () const;

      private:
        //If can_erase is false, current indexes the "next" value (must ++ to reach it)
        LN*             prev;            //prev should be initalized to the header
        LN*             current;         //current == prev->next
        LinkedPriorityQueue<T,tgt>* ref_pq;
        int             expected_mod_count;
        bool            can_erase = true;

        //Called in friends begin/end
        Iterator(LinkedPriorityQueue<T,tgt>* iterate_over, LN* initial);
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


    bool (*gt) (const T& a, const T& b); // The gt used by enqueue (from template or constructor)
    LN* front     =  new LN();
    int used      =  0;                  //Cache for number of values in linked list
    int mod_count =  0;                  //For sensing concurrent modification

    //Helper methods
    void delete_list(LN*& front);        //Deallocate all LNs, and set front's argument to nullptr;
};





////////////////////////////////////////////////////////////////////////////////
//
//LinkedPriorityQueue class and related definitions

//Destructor/Constructors

template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>::~LinkedPriorityQueue() {
  delete_list(front); //Including header node
}


template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>::LinkedPriorityQueue(bool (*cgt)(const T& a, const T& b))
	:gt (tgt ? tgt: cgt)
{
	if (gt == nullptr)
		throw TemplateFunctionError("LinkedPriorityQueue::default constructor: neither specified");
	if (tgt && cgt && tgt != cgt)
		throw TemplateFunctionError("LinkedPriorityQueue::default constructor: both specified and different");
}


template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>::LinkedPriorityQueue(const LinkedPriorityQueue<T,tgt>& to_copy, bool (*cgt)(const T& a, const T& b))
	:gt (tgt ? tgt: cgt){
	if (!gt)
		gt = to_copy.gt;
	if (tgt && cgt && tgt != cgt)
		throw TemplateFunctionError("LinkedPriorityQueue::copy constructor: both specified and different");
	if (gt == to_copy.gt){
		used = to_copy.used;
		if (to_copy.used > 0){
			for (LN *temp_copy = to_copy.front->next, *temp = front; temp_copy; temp_copy=temp_copy->next, temp=temp->next)
				temp->next = new LN(temp_copy->value);
		}
	}
	else{
		for (LN *copy_ln = to_copy.front->next; copy_ln; copy_ln = copy_ln->next)
			enqueue(copy_ln->value);
	}
}


template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>::LinkedPriorityQueue(const std::initializer_list<T>& il, bool (*cgt)(const T& a, const T& b))
	:gt(tgt ? tgt: cgt)
{
	if (gt == nullptr)
	    throw TemplateFunctionError("LinkedPriorityQueue::initializer_list constructor: neither specified");
	if (tgt != nullptr && cgt != nullptr && tgt != cgt)
	    throw TemplateFunctionError("LinkedPriorityQueue::initializer_list constructor: both specified and different");
	for (auto &i : il)
		enqueue(i);
}


template<class T, bool (*tgt)(const T& a, const T& b)>
template<class Iterable>
LinkedPriorityQueue<T,tgt>::LinkedPriorityQueue(const Iterable& i, bool (*cgt)(const T& a, const T& b))
	:gt(tgt ? tgt: cgt)
{
	if (gt == nullptr)
		throw TemplateFunctionError("ArrayPriorityQueue::Iterable constructor: neither specified");
	if (tgt != nullptr && cgt != nullptr && tgt != cgt)
	    throw TemplateFunctionError("ArrayPriorityQueue::Iterable constructor: both specified and different");
	for (auto item : i)
		enqueue(item);
}


////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class T, bool (*tgt)(const T& a, const T& b)>
bool LinkedPriorityQueue<T,tgt>::empty() const {
	return used == 0;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
int LinkedPriorityQueue<T,tgt>::size() const {
	return used;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
T& LinkedPriorityQueue<T,tgt>::peek () const {
	if (empty())
	    throw EmptyError("ArrayPriorityQueue::peek");
	return front->next->value;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
std::string LinkedPriorityQueue<T,tgt>::str() const {
	std::ostringstream result;
	result << "linked_queue[HEADER";
	for (LN* it = front->next; it; it=it->next)
		result << "->" << it->value;
	result << "](used=" << used << ",front=" << front <<
	",mod_count=" << mod_count << ")";
	return result.str();
}


////////////////////////////////////////////////////////////////////////////////
//
//Commands

template<class T, bool (*tgt)(const T& a, const T& b)>
int LinkedPriorityQueue<T,tgt>::enqueue(const T& element) {
	front->value = element;
	front = new LN(T(), front);
	for (LN *temp = front->next; temp && temp->next && !gt(temp->value, temp->next->value); temp=temp->next)
		std::swap(temp->value, temp->next->value);
	used++;
	mod_count++;
	return 1;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
T LinkedPriorityQueue<T,tgt>::dequeue() {
	if (this->empty())
	    throw EmptyError("ArrayPriorityQueue::dequeue");
	LN *temp = front->next;
	T to_return = temp->value;
	front->next = front->next->next;
	delete temp;
	--used;
	mod_count++;
	return to_return;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
void LinkedPriorityQueue<T,tgt>::clear() {
	delete_list(front->next);
	front->next = nullptr;
	mod_count++;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
template <class Iterable>
int LinkedPriorityQueue<T,tgt>::enqueue_all (const Iterable& i) {
	int count = 0;
	for (auto &it : i)
		enqueue(it), count++;
	return count;
}


////////////////////////////////////////////////////////////////////////////////
//
//Operators

template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>& LinkedPriorityQueue<T,tgt>::operator = (const LinkedPriorityQueue<T,tgt>& rhs) {
	if (this == &rhs)
	    return *this;
	LN *temp = front;
	for (LN *rhs_temp = rhs.front; rhs_temp->next; temp = temp->next, rhs_temp = rhs_temp->next){
		if (temp->next)
			temp->next->value = rhs_temp->next->value;
		else
			temp->next = new LN(rhs_temp->next->value);
	}
	if (temp->next)
		delete_list(temp->next);
	gt = rhs.gt;
	used = rhs.size();
	return *this;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
bool LinkedPriorityQueue<T,tgt>::operator == (const LinkedPriorityQueue<T,tgt>& rhs) const {
	if (this == &rhs) return true;
	if (gt != rhs.gt) return false;
	if (used != rhs.size()) return false;
	for (LN *rhs_temp = rhs.front->next, *temp = front->next; rhs_temp && temp; rhs_temp=rhs_temp->next, temp=temp->next)
		if (rhs_temp->value != temp->value) return false;
	return true;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
bool LinkedPriorityQueue<T,tgt>::operator != (const LinkedPriorityQueue<T,tgt>& rhs) const {
	return !(*this == rhs);
}


template<class T, bool (*tgt)(const T& a, const T& b)>
std::ostream& operator << (std::ostream& outs, const LinkedPriorityQueue<T,tgt>& pq) {
	outs << "priority_queue[";
	if (!pq.empty()){
		std::string temp;
		auto it = pq.begin();
		temp += *(it++);
		for (; it != pq.end(); ++it)
			temp += "," + *it;
		std::reverse(temp.begin(), temp.end());
		outs << temp;
	}
	outs << "]:highest";
	return outs;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors


template<class T, bool (*tgt)(const T& a, const T& b)>
auto LinkedPriorityQueue<T,tgt>::begin () const -> LinkedPriorityQueue<T,tgt>::Iterator {
	return Iterator(const_cast<LinkedPriorityQueue<T,tgt>*>(this),front->next);
}


template<class T, bool (*tgt)(const T& a, const T& b)>
auto LinkedPriorityQueue<T,tgt>::end () const -> LinkedPriorityQueue<T,tgt>::Iterator {
	return Iterator(const_cast<LinkedPriorityQueue<T,tgt>*>(this),nullptr);
}


////////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class T, bool (*tgt)(const T& a, const T& b)>
void LinkedPriorityQueue<T,tgt>::delete_list(LN*& front) {
	for (LN* temp = front; front; temp = front){
		front = front->next;
		delete temp;
	}
	used = 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//Iterator class definitions

template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>::Iterator::Iterator(LinkedPriorityQueue<T,tgt>* iterate_over, LN* initial)
	:current(initial), ref_pq(iterate_over), prev(iterate_over->front), expected_mod_count(ref_pq->mod_count)
{
}


template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>::Iterator::~Iterator()
{}


template<class T, bool (*tgt)(const T& a, const T& b)>
T LinkedPriorityQueue<T,tgt>::Iterator::erase() {
	if (expected_mod_count != ref_pq->mod_count)
		throw ConcurrentModificationError("LinkedPriorityQueue::Iterator::erase");
	if (!can_erase)
		throw CannotEraseError("LinkedPriorityQueue::Iterator::erase Iterator cursor already erased");
	can_erase = false;
	T to_return = current->value;
	if (prev == ref_pq->front){
		current = current->next;
		ref_pq->dequeue();
	}
	else{
		prev->next = current->next;
		delete current;
		current = prev->next;
		ref_pq->used--;
		ref_pq->mod_count++;
	}
	expected_mod_count = ref_pq->mod_count;
	return to_return;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
std::string LinkedPriorityQueue<T,tgt>::Iterator::str() const {
	std::ostringstream answer;
	answer << ref_pq->str() << "(current=" << current->value << ",expected_mod_count=" << expected_mod_count << ",can_erase=" << can_erase << ")";
	return answer.str();
}


template<class T, bool (*tgt)(const T& a, const T& b)>
auto LinkedPriorityQueue<T,tgt>::Iterator::operator ++ () -> LinkedPriorityQueue<T,tgt>::Iterator& {
	if (expected_mod_count != ref_pq->mod_count)
		throw ConcurrentModificationError("LinkedPriorityQueue::Iterator::operator ++");
	if (!current)
		can_erase = false;
	else if (can_erase)
		current = (prev = current)->next;
	else
		can_erase = true;
	return *this;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
auto LinkedPriorityQueue<T,tgt>::Iterator::operator ++ (int) -> LinkedPriorityQueue<T,tgt>::Iterator {
	if (expected_mod_count != ref_pq->mod_count)
		throw ConcurrentModificationError("LinkedPriorityQueue::Iterator::operator ++");
	Iterator to_return(*this);
	if (!current)
		can_erase = false;
	else if (can_erase)
		current = (prev = current)->next;
	else
		can_erase = true;
	return to_return;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
bool LinkedPriorityQueue<T,tgt>::Iterator::operator == (const LinkedPriorityQueue<T,tgt>::Iterator& rhs) const {
	const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
	if (rhsASI == 0)
		throw IteratorTypeError("LinkedPriorityQueue::Iterator::operator ==");
	if (expected_mod_count != ref_pq->mod_count)
		throw ConcurrentModificationError("LinkedPriorityQueue::Iterator::operator ==");
	if (ref_pq != rhsASI->ref_pq)
		throw ComparingDifferentIteratorsError("LinkedPriorityQueue::Iterator::operator ==");
	return current == rhsASI->current;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
bool LinkedPriorityQueue<T,tgt>::Iterator::operator != (const LinkedPriorityQueue<T,tgt>::Iterator& rhs) const {
	const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
	if (rhsASI == 0)
		throw IteratorTypeError("LinkedPriorityQueue::Iterator::operator !=");
	if (expected_mod_count != ref_pq->mod_count)
		throw ConcurrentModificationError("LinkedPriorityQueue::Iterator::operator !=");
	if (ref_pq != rhsASI->ref_pq)
		throw ComparingDifferentIteratorsError("LinkedPriorityQueue::Iterator::operator !=");
	return current != rhsASI->current;
}

template<class T, bool (*tgt)(const T& a, const T& b)>
T& LinkedPriorityQueue<T,tgt>::Iterator::operator *() const {
	if (expected_mod_count != ref_pq->mod_count)
		throw ConcurrentModificationError("LinkedPriorityQueue::Iterator::operator *");
	if (!can_erase || !current) {
		std::ostringstream where;
		where << current
			  << " when front = " << ref_pq->front->value;
		throw IteratorPositionIllegal("LinkedPriorityQueue::Iterator::operator * Iterator illegal: "+where.str());
	}
	return current->value;
}

template<class T, bool (*tgt)(const T& a, const T& b)>
T* LinkedPriorityQueue<T,tgt>::Iterator::operator ->() const {
	if (expected_mod_count != ref_pq->mod_count)
		throw ConcurrentModificationError("LinkedPriorityQueue::Iterator::operator ->");
	if (!can_erase || !current) {
		std::ostringstream where;
		where << current
			  << " when front = " << ref_pq->front->value;
		throw IteratorPositionIllegal("LinkedPriorityQueue::Iterator::operator -> Iterator illegal: "+where.str());
	}
	return &current->value;
}
}

#endif /* LINKED_PRIORITY_QUEUE_HPP_ */
