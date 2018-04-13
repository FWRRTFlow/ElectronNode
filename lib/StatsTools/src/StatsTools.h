#ifndef _STATS_TOOLS_H_
#define _STATS_TOOLS_H_

//int freeRam ()
//{
//    extern int __heap_start, *__brkval;
//    int v;
//    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
//}

//void PrintMem(char* mem, int len)
//{
//    Serial.println();
//    for (int i = 0; i < len; ++i)
//    {
//        Serial.print(mem[i] & 0xFF,HEX);Serial.print(" ");
//        if (i % 10 == 0)
//        {
//            Serial.println(">");
//        }
//    }
//    Serial.println();
//}

char statsToolsAlloc[200]={0}; // Allocate a RAM buffer to this library to solve array overlap

namespace StatsTools
{
    template <typename T>
    T fabs(T val)
    {
        return (val < 0) ? -val : val;
    }

    template <typename T>
    void InsertionSort(T arr[], int count)
    {
        for (int i = 1; i < count; ++i)
        {
            for (int j = i; j > 0; --j)
            {
                if (arr[j] < arr[j-1])
                {
                    T temp = arr[j];
                    arr[j] = arr[j-1];
                    arr[j-1] = temp;
                }
                else
                    break;
            }
        }
    }

    template <typename T>
    void BubbleSort(T arr[], int count)
    {
        for (int i = 1; i < count; ++i)
        {
            for (int j = 0; j < count - i; ++j)
            {
                if (arr[j] > arr[j+1])
                {
                    T temp = arr[j];
                    arr[j] = arr[j+1];
                    arr[j+1] = temp;
                }
            }
        }
    }

    template <typename T>
    T* SortedCopy(T arr[10], int count)
    {
        T *sortedArr = reinterpret_cast<T*>(statsToolsAlloc);
        for (int j = 1; j < count; j++)
        {
            sortedArr[j] = arr[j];
        }
        InsertionSort(sortedArr, count);
        return sortedArr;
    }

    template <typename T>
    float Average(T arr[], int count)
    {
        float total = 0.0;
        for (int i = 0; i < count; ++i) // Get the average of all readings
        {
          total += arr[i];
        }
        return total/count;
    }

    template <typename T>
    T Median(T arr[], int count, bool sorted = false)
    {
        T *sortedArr = (sorted) ? arr : SortedCopy(arr, count);

        T retVal = sortedArr[count/2];

        return retVal;
    }

    template <typename T>
    T MAD(T arr[], int count)
    {
        // Go ahead and get a copy of the array so it can be modified
        T *sortedArr = SortedCopy(arr, count);

        T median = Median(sortedArr, count, true);

        // Generate the deviations
        for (int i = 0; i < count; ++i)
        {
            sortedArr[i] = sortedArr[i] - median;
            if (sortedArr[i] < 0) // Absolute Value
                sortedArr[i] *= -1;
        }

        InsertionSort(sortedArr, count);
        T MAD = Median(sortedArr, count, true);

        return MAD;
    }

    template <typename T>
    void TrimOutliers(T arr[], unsigned int &count)
    {
        T mad = MAD(arr, count);
        if (StatsTools::fabs(mad) < .1) // Remove divide by zero errors
            mad = .1;

        T median = Median(arr, count);
        T *trimmedBuffer = reinterpret_cast<T*>(statsToolsAlloc);

        int countOut = 0;

        for (int i = 0; i < count; ++i)
        {
            float devs = (arr[i] - median) / (float)mad;
            if (StatsTools::fabs(devs) < 3.5)
            {
                trimmedBuffer[countOut++] = arr[i];
            }
        }

        memcpy(arr, trimmedBuffer, countOut*sizeof(T));
        count = countOut;
    }

}

#endif // _STATS_TOOLS_H_
