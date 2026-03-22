import time
import tracemalloc
import csv
import os
from copy import deepcopy


class SortInstrumentation:
    def __init__(self):
        self.comparisons = 0
        self.swaps = 0
        self.assignments = 0


class TimSort:
    """Python's built-in sorted (Timsort) - hybrid stable sort"""
    def __init__(self):
        self.inst = SortInstrumentation()

    def sort(self, arr):
        # Python built-in - just count as 1 operation for comparison
        return sorted(arr)


class MergeSort:
    def __init__(self):
        self.inst = SortInstrumentation()

    def sort(self, arr):
        self._merge_sort(arr, 0, len(arr) - 1)
        return arr

    def _merge_sort(self, arr, left, right):
        if left < right:
            mid = (left + right) // 2
            self._merge_sort(arr, left, mid)
            self._merge_sort(arr, mid + 1, right)
            self._merge(arr, left, mid, right)

    def _merge(self, arr, left, mid, right):
        left_part = arr[left:mid+1]
        right_part = arr[mid+1:right+1]
        
        i = j = 0
        k = left
        
        while i < len(left_part) and j < len(right_part):
            self.inst.comparisons += 1
            if left_part[i] <= right_part[j]:
                arr[k] = left_part[i]
                i += 1
            else:
                arr[k] = right_part[j]
                j += 1
            self.inst.assignments += 1
            k += 1
        
        while i < len(left_part):
            arr[k] = left_part[i]
            self.inst.assignments += 1
            i += 1
            k += 1
        
        while j < len(right_part):
            arr[k] = right_part[j]
            self.inst.assignments += 1
            j += 1
            k += 1


class QuickSort:
    def __init__(self):
        self.inst = SortInstrumentation()

    def sort(self, arr):
        self._quick_sort(arr, 0, len(arr) - 1)
        return arr

    def _quick_sort(self, arr, low, high):
        if low < high:
            pi = self._partition(arr, low, high)
            self._quick_sort(arr, low, pi - 1)
            self._quick_sort(arr, pi + 1, high)

    def _partition(self, arr, low, high):
        pivot = arr[high]
        i = low - 1
        
        for j in range(low, high):
            self.inst.comparisons += 1
            if arr[j] < pivot:
                i += 1
                arr[i], arr[j] = arr[j], arr[i]
                self.inst.swaps += 1
        
        arr[i + 1], arr[high] = arr[high], arr[i + 1]
        self.inst.swaps += 1
        return i + 1


class HeapSort:
    def __init__(self):
        self.inst = SortInstrumentation()

    def sort(self, arr):
        n = len(arr)
        
        # build max heap
        for i in range(n // 2 - 1, -1, -1):
            self._heapify(arr, n, i)
        
        # extract elements from heap
        for i in range(n - 1, 0, -1):
            arr[0], arr[i] = arr[i], arr[0]
            self.inst.swaps += 1
            self._heapify(arr, i, 0)
        
        return arr

    def _heapify(self, arr, n, i):
        largest = i
        left = 2 * i + 1
        right = 2 * i + 2
        
        if left < n:
            self.inst.comparisons += 1
            if arr[left] > arr[largest]:
                largest = left
        
        if right < n:
            self.inst.comparisons += 1
            if arr[right] > arr[largest]:
                largest = right
        
        if largest != i:
            arr[i], arr[largest] = arr[largest], arr[i]
            self.inst.swaps += 1
            self._heapify(arr, n, largest)


class InsertionSort:
    def __init__(self):
        self.inst = SortInstrumentation()

    def sort(self, arr):
        for i in range(1, len(arr)):
            key = arr[i]
            j = i - 1
            
            while j >= 0:
                self.inst.comparisons += 1
                if arr[j] > key:
                    arr[j + 1] = arr[j]
                    self.inst.assignments += 1
                    j -= 1
                else:
                    break
            
            arr[j + 1] = key
            self.inst.assignments += 1
        
        return arr


def benchmark_sort(sort_name, sort_cls, data, label=""):
    """Run one sorting algorithm and collect metrics"""
    # make a copy to avoid modifying original
    arr = deepcopy(data)
    
    sorter = sort_cls()
    
    tracemalloc.start()
    start_time = time.perf_counter()
    
    sorter.sort(arr)
    
    elapsed = time.perf_counter() - start_time
    current, peak = tracemalloc.get_traced_memory()
    tracemalloc.stop()
    
    return {
        'algorithm': sort_name,
        'size': len(data),
        'label': label,
        'time_s': elapsed,
        'comparisons': getattr(sorter.inst, 'comparisons', 0),
        'swaps': getattr(sorter.inst, 'swaps', 0),
        'assignments': getattr(sorter.inst, 'assignments', 0),
        'memory_peak_bytes': peak,
        'memory_per_element_bytes': peak / len(data) if data else 0,
    }


def run_sorting_benchmark(data, label=""):
    """Run all 5 sorting algorithms on data and return results"""
    algorithms = [
        ('TimSort (Built-in)', TimSort),
        ('MergeSort', MergeSort),
        ('QuickSort', QuickSort),
        ('HeapSort', HeapSort),
        ('InsertionSort', InsertionSort),
    ]
    
    results = []
    for algo_name, algo_cls in algorithms:
        print(f"  Running {algo_name}...")
        try:
            result = benchmark_sort(algo_name, algo_cls, data, label)
            results.append(result)
        except Exception as e:
            print(f"    Failed: {e}")
    
    return results


def save_results(results, output_file):
    """Save results to CSV"""
    if not results:
        return
    
    keys = list(results[0].keys())
    with open(output_file, 'w', newline='', encoding='utf-8') as f:
        w = csv.DictWriter(f, fieldnames=keys)
        w.writeheader()
        for r in results:
            w.writerow(r)
    
    print(f"Results saved to {output_file}")
