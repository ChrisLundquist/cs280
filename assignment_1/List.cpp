#include <stdlib.h>

template <class T>
List<T>::List() {
    head = NULL;
    tail = NULL;
    size_ = 0;
}

template <class T>
T List<T>::operator[](unsigned index) const {
    Node<T>* node = head;

    for(unsigned i = 0; i < index; i++){
        node = node->next;
    }
    return node->data;
}

template <class T>
unsigned List<T>::size() const {
    return size_;
}

template <class T>
void List<T>::push(T value) {
    size_++;

    Node<T>* new_node = new Node<T>();
    new_node->next = NULL;
    new_node->data = value;

    if(head == NULL) {
        head = new_node;
    } else {
        new_node->next = head;
        head = new_node;
    }
}

template <class T>
void List<T>::erase(unsigned index) {
    Node<T>* previous, target, next;

    if(index == 0) {
        pop();
        return;
    }

    previous = this[index - 1];
    target = previous->next;
    next = target->next;

    previous->next = next;

    delete target;
    size_--;
}

template <class T>
T List<T>::front() const {
    return head->data;
}

template <class T>
void List<T>::pop() {
    Node<T>* tmp = head;

    head = head->next;
    size_--;
    delete tmp;
}
