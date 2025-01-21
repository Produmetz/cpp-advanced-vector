/* Разместите здесь код класса Vector*/
#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <algorithm>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    RawMemory(const RawMemory&) = delete;
    
    RawMemory& operator=(const RawMemory& rhs) = delete;
    
    RawMemory(RawMemory&& other) noexcept {
        buffer_ = other.buffer_;
        capacity_ = other.capacity_;
        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
            rhs.Deallocate(rhs.buffer_);
            rhs.buffer_ = nullptr;
            rhs.capacity_ = 0;
        }
        return *this;
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

//Кажется, у меня в целом коряво получилось
template <typename T>
class Vector {
public:
    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }
    void Resize(size_t new_size){
        if(new_size < size_){
            std::destroy_n(data_.GetAddress() + new_size , size_ - new_size); // возможно мне тут придётся сдвинуть начало 
            //size_ = new_size;
        }else if(new_size > size_){
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + size_ , new_size - size_);
        }
        size_ = new_size;
    };


   
    template <typename Type>
    void PushBack(Type&& value) {
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (new_data + size_) T(std::forward<Type>(value));
            MoveOrCopyData(new_data, data_);
        }
        else {
            new (data_ + size_) T(std::forward<Type>(value));
        }
        ++size_;
    }


    
    /*void PushBack(T&& value){
        auto new_capacity = size_ + 1 <= data_.Capacity()? data_.Capacity() : size_ * 2;
        if(new_capacity == 0){
            new_capacity++; 
        }
        RawMemory<T> new_data(new_capacity);
        new (new_data + size_) T(value);

        // constexpr оператор if будет вычислен во время компиляции
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    
        ++size_;
    };*/
    void PopBack() /* noexcept */{
        //std::destroy(data_[size_ - 1], data_);
        std::destroy_n(data_.GetAddress() + size_ - 1, 1);
        --size_;
    };
    
    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                /* Применить copy-and-swap */
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            }
            else {
                /* Скопировать элементы из rhs, создав при необходимости новые
                   или удалив существующие */
                if (rhs.size_ < size_) {
                    std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + rhs.size_, data_.GetAddress());
                    std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
                }
                else {
                    std::copy(rhs.data_.GetAddress(), rhs.data_.GetAddress() + size_, data_.GetAddress());
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_);
                }
                size_ = rhs.size_;
            }
        }
        return *this;
    }

    Vector(Vector&& other) noexcept {
        Swap(other);
    }

    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
        }
        return *this;
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }
    using iterator = T*;
    using const_iterator = const T*;
    
    iterator begin() noexcept{
        return data_.GetAddress();
    };
    iterator end() noexcept{
        return data_.GetAddress() + size_;
    };
    const_iterator begin() const noexcept{
        return data_.GetAddress();
    };
    const_iterator end() const noexcept{
        return data_.GetAddress() + size_;
    };
    const_iterator cbegin() const noexcept{
        return begin();
    };
    const_iterator cend() const noexcept{
        return end();
    };

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args){
        size_t shift = std::distance(cbegin(), pos);
 
        if (size_ + 1 > Capacity()) {
            
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            
            new (new_data.GetAddress() + shift) T(std::forward<Args>(args)...);
 
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(begin(), shift, new_data.GetAddress());
                std::uninitialized_move_n(begin() + shift, size_ - shift, new_data.GetAddress() + shift + 1);
            
            } else {
                std::uninitialized_copy_n(begin(), shift, new_data.GetAddress());
                std::uninitialized_copy_n(begin() + shift, size_ - shift, new_data.GetAddress() + shift + 1);
            }
 
            std::destroy_n(begin(), size_);
            data_.Swap(new_data);
            
        } else {
            
            try {
                
                if (pos != end()) {
                    
                    T intermediate_object(std::forward<Args>(args)...);                   
                    new (end()) T(std::forward<T>(*(end() - 1)));
                    
                    std::move_backward(begin() + shift, end() - 1, end());                   
                    *(begin() + shift) = std::forward<T>(intermediate_object);
                    
                } else {
                    new (end()) T(std::forward<Args>(args)...);
                }
                
            } catch (...) {
                operator delete (end());
            throw;
            }
        }
        ++size_;
        return begin() + shift;
    };
    iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/{

        std::move(begin() + (pos - begin()) + 1, end(), begin() + (pos - begin()));
        std::destroy_at(end() - 1);
        --size_;
        return begin() + (pos - begin());
    };
    iterator Insert(const_iterator pos, const T& value){
        return Emplace(pos, value);
    };
    iterator Insert(const_iterator pos, T&& value){
        return Emplace(pos, std::move(value));
    };

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        // constexpr оператор if будет вычислен во время компиляции
        MoveOrCopyData(new_data, data_);
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }
    template <typename... Args>
    T& EmplaceBack(Args&&... args){
        
        if(size_ + 1 > Capacity()){
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (new_data + size_) T(std::forward<Args>(args)...);
            MoveOrCopyData(new_data, data_);
        }else{
            new (data_ + size_) T(std::forward<Args>(args)...);
        }
        ++size_;
        return (*this)[size_ - 1];
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;


    void MoveOrCopyData(RawMemory<T> &new_data, RawMemory<T> &data){
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data.GetAddress(), size_, new_data.GetAddress());
        }else{
            std::uninitialized_copy_n(data.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data.GetAddress(), size_);
        data.Swap(new_data);
    }
};
