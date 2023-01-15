#pragma once

#include "array_ptr.h"

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iostream>

using namespace std::string_literals;

class ReserveProxyObj {
public:
    explicit ReserveProxyObj(size_t value)
        : value_(value)
    {}

    size_t GetValue() const {
        return value_;
    }

private:
    size_t value_ = 0;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных
    // значением по умолчанию
    explicit SimpleVector(size_t size)
        : SimpleVector(size, Type())
    {}

    // Создаёт вектор из size элементов, инициализированных
    // значением value
    SimpleVector(size_t size, const Type& value)
        : capacity_(size), size_(size), raw_vector_(size)
    {
        std::fill(begin(), end(), value);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) {
        Assign(init.begin(), init.end());
    }

    SimpleVector(const SimpleVector& other) {
        *this = other;
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            Assign(rhs.begin(), rhs.end());
        }
        return *this;
    }

    SimpleVector(SimpleVector&& other) {
        *this = std::move(other);
    }

    SimpleVector& operator=(SimpleVector&& rhs) {
        if (this != &rhs) {
            capacity_ = std::exchange(rhs.capacity_, 0);
            size_ = std::exchange(rhs.size_, 0);
            raw_vector_.Delete();
            raw_vector_.swap(rhs.raw_vector_);
        }
        return *this;
    }

    SimpleVector(const ReserveProxyObj& Rsrv) {
        Reserve(Rsrv.GetValue());
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return raw_vector_[index];
    }

    // Возвращает константную ссылку на элемент с индексом
    // index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return raw_vector_[index];
    }

    // Возвращает константную ссылку на элемент с индексом
    // index
    // Выбрасывает исключение std::out_of_range, если
    // index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Out of range!");
        }
        return raw_vector_[index];
    }

    // Возвращает константную ссылку на элемент с индексом
    // index
    // Выбрасывает исключение std::out_of_range, если
    // index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Out of range!");
        }
        return raw_vector_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ResizeCapacity(new_capacity);
        }
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение
    // по умолчанию для типа Type
    void Resize(size_t new_size) {
         if (new_size <= size_) {
            size_ = new_size;
            return;
        }
        if (new_size <= capacity_) {
            for (auto it = &raw_vector_[size_];
                      it != &raw_vector_[new_size]; ++it) {
                *it = std::move(Type{});                  
            }
            size_ = new_size;
            return;
        }
        ResizeCapacity(std::max(new_size, 2 * capacity_));
        size_ = new_size;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& value) {
        if (capacity_ == size_) {
            ResizeCapacity(size_ == 0 ? 1 : size_ * 2);
        }
        raw_vector_[size_++] = value;
    }

    void PushBack(Type&& value) {
        if (capacity_ == size_) {
            ResizeCapacity(size_ == 0 ? 1 : size_ * 2);
        }
        raw_vector_[size_++] = std::move(value);
    }

    // "Удаляет" последний элемент вектора. Вектор не должен
    // быть пустым
    void PopBack() noexcept {
        assert(!IsEmpty());
        --size_;
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен
    // полностью, вместимость вектора должна увеличиться
    // вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= begin() && pos <= end());
        Iterator it = const_cast<Iterator>(pos);
        if (capacity_ > size_) {
            std::move_backward(std::make_move_iterator(it),
                               std::make_move_iterator(end()),
                               &raw_vector_[size_ + 1]);
            *it = std::move(value);
            ++size_;
            return it;
        }
        if (capacity_ == 0) {
            ArrayPtr<Type> tmp_raw_vector(1);
            tmp_raw_vector[0] = std::move(value);
            raw_vector_ = std::move(tmp_raw_vector);
            capacity_ = 1;
            size_ = 1;
            return &raw_vector_[0];
        }
        size_t new_capacity = size_ * 2;
        size_t insert_index = std::distance(begin(), it);
        ArrayPtr<Type> tmp_raw_vector(new_capacity);
        std::move(std::make_move_iterator(begin()),
                  std::make_move_iterator(it),
                  &tmp_raw_vector[0]);
        tmp_raw_vector[insert_index] = std::move(value);
        std::move(std::make_move_iterator(it),
                  std::make_move_iterator(end()),
                  &tmp_raw_vector[insert_index + 1]);
        raw_vector_ = std::move(tmp_raw_vector);
        capacity_ = new_capacity;
        ++size_;
        return &raw_vector_[insert_index];
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= begin() && pos <= end());
        Iterator it = const_cast<Iterator>(pos);
        if (capacity_ > size_) {
            std::move_backward(std::make_move_iterator(it),
                               std::make_move_iterator(end()),
                               &raw_vector_[size_ + 1]);
            *it = std::move(value);
            ++size_;
            return it;
        }
        if (capacity_ == 0) {
            ArrayPtr<Type> tmp_raw_vector(1);
            tmp_raw_vector[0] = std::move(value);
            raw_vector_ = std::move(tmp_raw_vector);
            capacity_ = 1;
            size_ = 1;
            return &raw_vector_[0];
        }
        size_t new_capacity = size_ * 2;
        size_t insert_index = std::distance(begin(), it);
        ArrayPtr<Type> tmp_raw_vector(new_capacity);
        std::move(std::make_move_iterator(begin()),
                  std::make_move_iterator(it),
                  &tmp_raw_vector[0]);
        tmp_raw_vector[insert_index] = std::move(value);
        std::move(std::make_move_iterator(it),
                  std::make_move_iterator(end()),
                  &tmp_raw_vector[insert_index + 1]);
        raw_vector_ = std::move(tmp_raw_vector);
        capacity_ = new_capacity;
        ++size_;
        return &raw_vector_[insert_index];
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos < end());
        Iterator it = const_cast<Iterator>(pos);
        size_t index = std::distance(begin(), it);
        if (size_ > index + 1) {
            for (auto i = index; i < size_ - 1; ++i) {
                raw_vector_[i] = std::move(raw_vector_[i + 1]);
            }
        }
        --size_;
        return it;
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен)
    // nullptr
    Iterator begin() noexcept {
        return raw_vector_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен)
    // nullptr
    Iterator end() noexcept {
        return &raw_vector_[size_];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен)
    // nullptr
    ConstIterator begin() const noexcept {
        return raw_vector_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен)
    // nullptr
    ConstIterator end() const noexcept {
        return &raw_vector_[size_];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен)
    // nullptr
    ConstIterator cbegin() const noexcept {
        return const_cast<Type*>(raw_vector_.Get());
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен)
    // nullptr
    ConstIterator cend() const noexcept {
        return &raw_vector_[size_];
    }

    void swap(SimpleVector& other) noexcept {
        raw_vector_.swap(other.raw_vector_);
        std::swap(capacity_, other.capacity_);
        std::swap(size_, other.size_);
    }

    ~SimpleVector()
    {}

private:
    size_t capacity_ = 0;
    size_t size_ = 0;
    ArrayPtr<Type> raw_vector_;

    template <typename InputIterator>
    void Assign(InputIterator s, InputIterator w) {
        size_t size = std::distance(s, w);
        SimpleVector<Type> tmp(size);
        if (size > 0) {
            std::move(std::make_move_iterator(s),
                      std::make_move_iterator(w), tmp.begin());
        }
        swap(tmp);
    }

    void ResizeCapacity(size_t new_capacity) {
        ArrayPtr<Type> tmp_raw_vector(new_capacity);
        std::move(std::make_move_iterator(begin()),
                  std::make_move_iterator(end()), &tmp_raw_vector[0]);
        raw_vector_ = std::move(tmp_raw_vector);
        capacity_ = new_capacity;
    }

};

template <typename Type>
void swap(SimpleVector<Type>& lhs,
          SimpleVector<Type>& rhs) noexcept {
    lhs.swap(rhs);
}

template <typename Type>
bool operator==(const SimpleVector<Type>& lhs,
                const SimpleVector<Type>& rhs) {
    return (&lhs == &rhs)  // Оптимизация сравнения списка
                           // с собой
        || (lhs.GetSize() == rhs.GetSize()
            && std::equal(lhs.begin(), lhs.end(),
                          rhs.begin())); // может бросить
                                         // исключение
}

template <typename Type>
bool operator!=(const SimpleVector<Type>& lhs,
                const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);  // может бросить исключение
}

template <typename Type>
bool operator<(const SimpleVector<Type>& lhs,
               const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(),
                                        rhs.begin(), rhs.end());
    // может бросить исключение
}

template <typename Type>
bool operator<=(const SimpleVector<Type>& lhs,
                const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);  // Может бросить исключение
}

template <typename Type>
bool operator>(const SimpleVector<Type>& lhs,
               const SimpleVector<Type>& rhs) {
    return (rhs < lhs);  // Может бросить исключение
}

template <typename Type>
bool operator>=(const SimpleVector<Type>& lhs,
                const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);  // Может бросить исключение
}
