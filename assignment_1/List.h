template <class T>
struct Node {
    Node* next;
    T data;
};

template <class T>
class List {
    private:
             unsigned size_;
             Node<T>* head;
             Node<T>* tail;
    public:
             List<T>();
             T operator[](unsigned index) const;
             unsigned size() const;
             void push(T value);
             void erase(unsigned index);
             T front() const;
             void pop();
};

#include "List.cpp"
