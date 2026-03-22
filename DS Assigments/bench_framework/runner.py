import time
import tracemalloc
import csv
import os
from collections import defaultdict

from bench_framework import data_loader
from bench_framework.structures import (
    DynamicArray,
    SinglyLinkedList,
    DoublyLinkedList,
    BST,
    AVL,
    BTree,
    RedBlackTree,
    HashTableChaining,
    BinaryHeap,
)

OUTPUT = os.path.join(os.getcwd(), "bench_output")
os.makedirs(OUTPUT, exist_ok=True)


def time_op(fn, *a, repeat=1):
    start = time.perf_counter()
    for _ in range(repeat):
        fn(*a)
    return (time.perf_counter() - start) / repeat


def measure_structure(name, cls, keys, sizes, samples=1000):
    results = []
    for n in sizes:
        arr = cls() if callable(cls) else cls
        keys_n = list(keys[:n])

        tracemalloc.start()
        t_insert = time_op(lambda: [arr.insert(k) for k in keys_n])
        current, peak = tracemalloc.get_traced_memory()
        tracemalloc.stop()

        # sample searches
        search_keys = keys_n[:min(samples, len(keys_n))]
        t_search = time_op(lambda: [arr.search(k) for k in search_keys])

        # sample deletes
        del_keys = keys_n[-min(samples, len(keys_n)):]
        t_delete = time_op(lambda: [arr.delete(k) for k in del_keys])

        # traversal
        t_trav = time_op(lambda: arr.traverse())

        meta = {
            'structure': name,
            'n': n,
            'insert_s': t_insert,
            'search_s': t_search,
            'delete_s': t_delete,
            'traverse_s': t_trav,
            'mem_peak_bytes': peak,
        }

        # structural metrics where available
        if hasattr(arr, 'height'):
            try:
                meta['height'] = arr.height()
            except Exception:
                meta['height'] = None
        if hasattr(arr, 'inst'):
            meta['comparisons'] = getattr(arr.inst, 'comparisons', 0)
            meta['rotations'] = getattr(arr.inst, 'rotations', 0)
            meta['collisions'] = getattr(arr.inst, 'collisions', 0)
        if hasattr(arr, 'load_factor'):
            try:
                meta['load_factor'] = arr.load_factor()
            except Exception:
                meta['load_factor'] = None

        results.append(meta)

    return results


def run_all(dset_key, sizes=[10000, 50000, 100000, 200000], structures=None):
    keys = list(data_loader.load_dataset(dset_key).tolist())
    # cap sizes to available
    available = len(keys)
    sizes = [s for s in sizes if s <= max(sizes) and s <= max(available, 1)]
    sizes = sorted(set(min(s, available) for s in sizes))

    default_structs = [
        ("DynamicArray", DynamicArray),
        ("SinglyLinkedList", SinglyLinkedList),
        ("DoublyLinkedList", DoublyLinkedList),
        ("BST", BST),
        ("AVL", AVL),
        ("BTree", lambda: BTree(t=4)),
        ("RedBlackTree", RedBlackTree),
        ("HashTable", lambda: HashTableChaining(capacity=16384)),
        ("BinaryHeap", BinaryHeap),
    ]
    structures = structures or default_structs

    all_results = []
    for name, cls in structures:
        print(f"Running {name} on {dset_key} sizes {sizes}...")
        res = measure_structure(name, cls, keys, sizes)
        all_results.extend(res)

    out_csv = os.path.join(OUTPUT, f"results_{dset_key}.csv")
    # collect all unique fieldnames across all results
    all_keys = set()
    for r in all_results:
        all_keys.update(r.keys())
    keys_order = sorted(all_keys)
    with open(out_csv, 'w', newline='', encoding='utf-8') as f:
        w = csv.DictWriter(f, fieldnames=keys_order, restval='')
        w.writeheader()
        for r in all_results:
            w.writerow(r)

    print(f"Results written to {out_csv}")
    return out_csv


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description="Run benchmarks on selected data structures and datasets")
    parser.add_argument('--structures', '-s', nargs='+', help="Names of structures to run, e.g. DynamicArray BST")
    parser.add_argument('--datasets', '-d', nargs='+', help="Dataset keys to run (random, nearly_sorted, duplicate_heavy, sorted, skewed, students)")
    args = parser.parse_args()

    struct_map = {
        'DynamicArray': DynamicArray,
        'SinglyLinkedList': SinglyLinkedList,
        'DoublyLinkedList': DoublyLinkedList,
        'BST': BST,
        'AVL': AVL,
        'BTree': lambda: BTree(t=4),
        'RedBlackTree': RedBlackTree,
        'HashTable': lambda: HashTableChaining(capacity=16384),
        'BinaryHeap': BinaryHeap,
    }

    ds_list = args.datasets or ['random', 'nearly_sorted', 'duplicate_heavy', 'sorted', 'skewed', 'students']
    for k in ds_list:
        try:
            if args.structures:
                chosen = []
                for name in args.structures:
                    if name in struct_map:
                        chosen.append((name, struct_map[name]))
                run_all(k, structures=chosen)
            else:
                run_all(k)
        except Exception as e:
            print(f"Failed for {k}: {e}")
