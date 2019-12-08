#pragma once
#include <vector>
#include <functional>
#include "byte.hpp"

//TODO: <=79 character lines

template <typename DataType = u8, typename AddressType = u16>
class MappedMemoryValue;

template <typename DataType = u8, typename AddressType = u16>
class MappedMemoryIterator;

template <typename DataType = u8, typename AddressType = u16>
class MappedMemory {
    public:
        friend class MappedMemoryIterator<DataType, AddressType>;
        friend class MappedMemoryValue<DataType, AddressType>;

        std::vector<DataType> memory;
        std::map<
                AddressType, 
                std::function<DataType(
                        MappedMemory* const mappedMemory,
                        const AddressType address
                )>
        > readFunctions;
        std::map<
                AddressType,
                std::function<void(
                        MappedMemory* const mappedMemory,
                        const AddressType address,
                        const DataType data
                )>
        > writeFunctions; 

        void resize(const size_t size) {
            memory.resize(size);
        }

        MappedMemoryIterator<DataType, AddressType> begin() {
            return MappedMemoryIterator<DataType, AddressType>(this, 0);
        }

        MappedMemoryValue<DataType, AddressType> operator[] (
                const AddressType address
        ) {
            return MappedMemoryValue<DataType, AddressType>(this, address);
        }

        MappedMemory(const size_t size) { 
            resize(size);
        }
};


template <typename DataType, typename AddressType>
class MappedMemoryValue {
    private:
        MappedMemory<DataType, AddressType>* const mappedMemory;
        const AddressType valueAddress;

    public:
        operator DataType() const {
            return mappedMemory->readFunctions.lower_bound(valueAddress)
                    ->second(mappedMemory, valueAddress);
        }
        void operator= (const DataType data) const {
            mappedMemory->writeFunctions.lower_bound(valueAddress)
                    ->second(mappedMemory, valueAddress, data);
        }

        MappedMemoryValue(
                decltype(mappedMemory) mappedMemory,
                decltype(valueAddress) valueAddress
        )     : mappedMemory{mappedMemory}, valueAddress{valueAddress} {
        }
};
            

template <typename DataType, typename AddressType>
class MappedMemoryIterator {
    private:
        MappedMemory<DataType, AddressType>* mappedMemory; 
        AddressType currentAddress;
    
    public:
        void copy(const MappedMemoryIterator& source) { 
            mappedMemory = source.mappedMemory;
            currentAddress = source.currentAddress;
        }

        MappedMemoryIterator(
                decltype(mappedMemory) const mappedMemory,
                const AddressType currentAddress 
        )     : mappedMemory{mappedMemory}, 
                currentAddress{currentAddress} {
        }

        MappedMemoryIterator(const MappedMemoryIterator& source) {
            copy(source);
        }

        MappedMemoryIterator& operator= (const MappedMemoryIterator& source) {
            copy(source);
            return *this;
        }

        MappedMemoryValue<DataType, AddressType> operator* () const {
            return MappedMemoryValue<DataType, AddressType>(
                    mappedMemory, currentAddress
            );
        }

        MappedMemoryValue<DataType, AddressType> operator[] 
                (const AddressType offset) const {
            return MappedMemoryValue<DataType, AddressType>(
                    mappedMemory, currentAddress + offset
            );
        } 

        friend bool operator== ( 
                const MappedMemoryIterator& left, 
                const MappedMemoryIterator& right
        ) {
            return (left.mappedMemory == right.mappedMemory 
                 && left.currentAddress == right.currentAddress
            ); 
        }
        friend bool operator!= (
                const MappedMemoryIterator& left,  
                const MappedMemoryIterator& right
        ) {
            return (left.mappedMemory != right.mappedMemory
                 || left.currentAddress != right.currentAddress
            );
        }
        friend bool operator> (
                const MappedMemoryIterator& left,  
                const MappedMemoryIterator& right
        ) {
            return left.currentAddress > right.currentAddress;
        }
        friend bool operator>= (
                const MappedMemoryIterator& left,  
                const MappedMemoryIterator& right
        ) {
            return left.currentAddress >= right.currentAddress; 
        }
        friend bool operator< (
                const MappedMemoryIterator& left,  
                const MappedMemoryIterator& right
        ) {
            return left.currentAddress < right.currentAddress;
        }
        friend bool operator<= (
                const MappedMemoryIterator& left,  
                const MappedMemoryIterator& right
        ) {
            return left.currentAddress <= right.currentAddress;
        }

        MappedMemoryIterator& operator++ () {
            ++currentAddress;
            return *this;
        }
        MappedMemoryIterator& operator-- () {
            --currentAddress;
            return *this;
        }
        MappedMemoryIterator operator++ (int) {
            auto old {*this};
            ++currentAddress;
            return old;
        }
        MappedMemoryIterator operator-- (int) {
            auto old {*this};
            --currentAddress;
            return old;
        }

        MappedMemoryIterator& operator+= (const AddressType offset) {
            currentAddress += offset;
            return *this;
        }
        MappedMemoryIterator& operator-= (const AddressType offset) {
            currentAddress -= offset;
            return *this; 
        }
        
        friend MappedMemoryIterator operator+ (
                const MappedMemoryIterator& left, 
                const AddressType right
        ) { 
            return MappedMemoryIterator(left.mappedMemory, left.currentAddress + right);
        }
        friend MappedMemoryIterator operator+ (
                const AddressType left,
                const MappedMemoryIterator& right
        ) {
            return MappedMemoryIterator(right.mappedMemory, right.currentAddress + left);
        }
        friend MappedMemoryIterator operator- (
                const MappedMemoryIterator& left,
                const AddressType right
        ) {
            return MappedMemoryIterator(left.mappedMemory, left.currentAddress - right);
        }
};
