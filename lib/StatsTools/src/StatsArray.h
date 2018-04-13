#ifndef _STATS_ARRAY_H_
#define _STATS_ARRAY_H_

#include "StatsTools.h"

namespace StatsTools
{
template <typename T>
class StatsArray
{
public:
    StatsArray(unsigned int startingSize) :
        size_(startingSize),
        values_(new T[size_]),
        count_(0) {}

    virtual ~StatsArray(){}

    T At(const unsigned int index)
    {
        return (index < count_) ? values_[index] : 0;
    }

    unsigned int Size()
    {
        return size_;
    }

    unsigned int Count()
    {
        return count_;
    }

    bool Append(const T value)
    {
        if (count_ < size_)
        {
            values_[count_] = value;
            count_++;
            return true;
        }
        // resize here?

        return false;
    }

    float Average()
    {
        return StatsTools::Average(values_, count_);
    }

    T Median()
    {
        return StatsTools::Median(values_, count_, false);
    }

    // WARNING: Permanently changes the order of the values
    void Sort()
    {
        StatsTools::InsertionSort(values_, count_);
    }

    void TrimOutliers()
    {
        // count will be copied into the function and also be modified
        StatsTools::TrimOutliers(values_, count_);
    }

    void Clear()
    {
        count_ = 0; // All values must now be replaced
    }

private:
    T *values_;
    unsigned int size_;
    unsigned int count_;

};

}

#endif // _STATS_ARRAY_H_
