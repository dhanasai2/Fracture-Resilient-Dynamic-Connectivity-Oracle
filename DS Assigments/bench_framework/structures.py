import math
import heapq


class Instrumentation:
    def __init__(self):
        self.comparisons = 0
        self.rotations = 0
        self.collisions = 0


class DynamicArray:
    def __init__(self):
        self.data = []
        self.inst = Instrumentation()

    def insert(self, x):
        self.data.append(x)

    def search(self, x):
        for v in self.data:
            self.inst.comparisons += 1
            if v == x:
                return True
        return False

    def delete(self, x):
        try:
            self.data.remove(x)
            return True
        except ValueError:
            return False

    def traverse(self):
        for v in self.data:
            pass


class SinglyLinkedList:
    class Node:
        def __init__(self, v, nxt=None):
            self.v = v
            self.next = nxt

    def __init__(self):
        self.head = None
        self.inst = Instrumentation()

    def insert_head(self, x):
        self.head = SinglyLinkedList.Node(x, self.head)
    
    # uniform API
    def insert(self, x):
        return self.insert_head(x)

    def search(self, x):
        cur = self.head
        while cur:
            self.inst.comparisons += 1
            if cur.v == x:
                return True
            cur = cur.next
        return False

    def delete(self, x):
        cur = self.head
        prev = None
        while cur:
            self.inst.comparisons += 1
            if cur.v == x:
                if prev:
                    prev.next = cur.next
                else:
                    self.head = cur.next
                return True
            prev = cur
            cur = cur.next
        return False

    def traverse(self):
        cur = self.head
        while cur:
            cur = cur.next
    
    # alias for uniform API
    remove = delete


class DoublyLinkedList:
    class Node:
        def __init__(self, v):
            self.v = v
            self.next = None
            self.prev = None

    def __init__(self):
        self.head = None
        self.tail = None
        self.inst = Instrumentation()

    def insert_head(self, x):
        n = DoublyLinkedList.Node(x)
        n.next = self.head
        if self.head:
            self.head.prev = n
        else:
            self.tail = n
        self.head = n
    
    # uniform API
    def insert(self, x):
        return self.insert_head(x)

    def search(self, x):
        cur = self.head
        while cur:
            self.inst.comparisons += 1
            if cur.v == x:
                return True
            cur = cur.next
        return False

    def delete(self, x):
        cur = self.head
        while cur:
            self.inst.comparisons += 1
            if cur.v == x:
                if cur.prev:
                    cur.prev.next = cur.next
                else:
                    self.head = cur.next
                if cur.next:
                    cur.next.prev = cur.prev
                else:
                    self.tail = cur.prev
                return True
            cur = cur.next
        return False
    
    # alias for uniform API
    remove = delete

    def traverse(self):
        cur = self.head
        while cur:
            cur = cur.next


class BSTNode:
    def __init__(self, k):
        self.k = k
        self.left = None
        self.right = None


class BST:
    def __init__(self):
        self.root = None
        self.inst = Instrumentation()

    def insert(self, k):
        if self.root is None:
            self.root = BSTNode(k)
            return
        cur = self.root
        while True:
            self.inst.comparisons += 1
            if k < cur.k:
                if cur.left is None:
                    cur.left = BSTNode(k)
                    return
                cur = cur.left
            else:
                if cur.right is None:
                    cur.right = BSTNode(k)
                    return
                cur = cur.right

    def search(self, k):
        cur = self.root
        while cur:
            self.inst.comparisons += 1
            if k == cur.k:
                return True
            cur = cur.left if k < cur.k else cur.right
        return False

    def _min_node(self, node):
        cur = node
        while cur.left:
            cur = cur.left
        return cur

    def delete(self, k):
        # iterative delete to avoid recursion depth issues
        parent = None
        node = self.root
        while node and node.k != k:
            parent = node
            if k < node.k:
                node = node.left
            else:
                node = node.right
        if node is None:
            return False

        # node with two children: find successor
        if node.left and node.right:
            succ_parent = node
            succ = node.right
            while succ.left:
                succ_parent = succ
                succ = succ.left
            node.k = succ.k
            # now delete succ which has at most one child
            node = succ
            parent = succ_parent

        # node has at most one child
        child = node.left if node.left else node.right
        if parent is None:
            self.root = child
        elif parent.left is node:
            parent.left = child
        else:
            parent.right = child
        return True

    def traverse(self):
        # iterative inorder traversal
        stack = []
        cur = self.root
        while stack or cur:
            while cur:
                stack.append(cur)
                cur = cur.left
            cur = stack.pop()
            # visit cur.k
            cur = cur.right

    def height(self):
        # iterative BFS to compute height
        if not self.root:
            return 0
        q = [(self.root, 1)]
        max_h = 0
        while q:
            node, h = q.pop(0)
            if h > max_h:
                max_h = h
            if node.left:
                q.append((node.left, h + 1))
            if node.right:
                q.append((node.right, h + 1))
        return max_h


class AVL(BST):
    class AVLNode(BSTNode):
        def __init__(self, k):
            super().__init__(k)
            self.height = 1

    def __init__(self):
        super().__init__()
        self.root = None
        # override instrumentation
        self.inst = Instrumentation()

    def _height(self, n):
        return n.height if n else 0

    def _update(self, n):
        n.height = 1 + max(self._height(n.left), self._height(n.right))

    def _balance_factor(self, n):
        return self._height(n.left) - self._height(n.right)

    def _rotate_right(self, y):
        # safe rotate: if y or its left child is None, nothing to do
        if y is None or y.left is None:
            return y
        x = y.left
        t2 = x.right
        x.right = y
        y.left = t2
        self._update(y)
        self._update(x)
        self.inst.rotations += 1
        return x

    def _rotate_left(self, x):
        # safe rotate: if x or its right child is None, nothing to do
        if x is None or x.right is None:
            return x
        y = x.right
        t2 = y.left
        y.left = x
        x.right = t2
        self._update(x)
        self._update(y)
        self.inst.rotations += 1
        return y

    def insert(self, k):
        def _ins(node, k):
            if not node:
                return AVL.AVLNode(k)
            self.inst.comparisons += 1
            if k < node.k:
                node.left = _ins(node.left, k)
            else:
                node.right = _ins(node.right, k)
            self._update(node)
            bf = self._balance_factor(node)
            if bf > 1 and node.left is not None and k < node.left.k:
                return self._rotate_right(node)
            if bf < -1 and node.right is not None and k > node.right.k:
                return self._rotate_left(node)
            if bf > 1 and node.left is not None and k > node.left.k:
                node.left = self._rotate_left(node.left)
                return self._rotate_right(node)
            if bf < -1 and node.right is not None and k < node.right.k:
                # guard against None right
                node.right = self._rotate_right(node.right)
                return self._rotate_left(node)
            return node

        self.root = _ins(self.root, k)

    def height(self):
        return super().height()


class HashTableChaining:
    def __init__(self, capacity=1024):
        self.buckets = [[] for _ in range(capacity)]
        self.capacity = capacity
        self.size = 0
        self.inst = Instrumentation()

    def _bucket(self, k):
        return hash(k) % self.capacity

    def insert(self, k):
        b = self._bucket(k)
        if len(self.buckets[b]) > 0:
            self.inst.collisions += 1
        self.buckets[b].append(k)
        self.size += 1

    def search(self, k):
        b = self._bucket(k)
        for v in self.buckets[b]:
            self.inst.comparisons += 1
            if v == k:
                return True
        return False

    def delete(self, k):
        b = self._bucket(k)
        try:
            self.buckets[b].remove(k)
            self.size -= 1
            return True
        except ValueError:
            return False

    def load_factor(self):
        return self.size / self.capacity
    
    def traverse(self):
        for bucket in self.buckets:
            for v in bucket:
                pass


class BinaryHeap:
    def __init__(self):
        self._h = []
        self.inst = Instrumentation()

    def insert(self, k):
        heapq.heappush(self._h, k)

    def extract_min(self):
        return heapq.heappop(self._h) if self._h else None

    def search(self, k):
        for v in self._h:
            self.inst.comparisons += 1
            if v == k:
                return True
        return False

    def traverse(self):
        for v in list(self._h):
            pass


# Wrapper so heapq comparisons increment our instrumentation counter
class _CmpWrapper:
    def __init__(self, key, inst):
        self.key = key
        self._inst = inst

    def __lt__(self, other):
        if self._inst:
            self._inst.comparisons += 1
        return self.key < other.key

    def __eq__(self, other):
        return self.key == other.key


class BinaryHeap:
    def __init__(self):
        self._h = []
        self.inst = Instrumentation()

    def insert(self, k):
        heapq.heappush(self._h, _CmpWrapper(k, self.inst))

    def extract_min(self):
        w = heapq.heappop(self._h) if self._h else None
        return w.key if w else None

    def search(self, k):
        for v in self._h:
            # count a comparison per examined element
            self.inst.comparisons += 1
            if v.key == k:
                return True
        return False

    def traverse(self):
        for v in list(self._h):
            pass


# new heap implementations requested by user
class BinomialHeap:
    class Node:
        def __init__(self, key):
            self.key = key
            self.degree = 0
            self.parent = None
            self.child = None
            self.sibling = None

    def __init__(self):
        self.head = None
        self.inst = Instrumentation()

    def _link(self, y, z):
        # make y a child of z
        y.parent = z
        y.sibling = z.child
        z.child = y
        z.degree += 1
        # no comparisons recorded here beyond earlier

    def _merge_roots(self, h1, h2):
        # merge two root lists by degree ascending
        if h1 is None:
            return h2
        if h2 is None:
            return h1
        if h1.degree <= h2.degree:
            head = h1
            h1 = h1.sibling
        else:
            head = h2
            h2 = h2.sibling
        tail = head
        while h1 and h2:
            if h1.degree <= h2.degree:
                tail.sibling = h1
                h1 = h1.sibling
            else:
                tail.sibling = h2
                h2 = h2.sibling
            tail = tail.sibling
        tail.sibling = h1 if h1 else h2
        return head

    def _union(self, other):
        self.head = self._merge_roots(self.head, other.head)
        if self.head is None:
            return
        prev = None
        curr = self.head
        nxt = curr.sibling
        while nxt:
            if (curr.degree != nxt.degree) or \
               (nxt.sibling and nxt.sibling.degree == curr.degree):
                prev = curr
                curr = nxt
            else:
                # compare keys to decide which tree to link
                self.inst.comparisons += 1
                if curr.key <= nxt.key:
                    curr.sibling = nxt.sibling
                    self._link(nxt, curr)
                else:
                    if prev:
                        prev.sibling = nxt
                    else:
                        self.head = nxt
                    self._link(curr, nxt)
                    curr = nxt
                # end comparison
            nxt = curr.sibling

    def insert(self, k):
        newheap = BinomialHeap()
        newheap.head = BinomialHeap.Node(k)
        self._union(newheap)

    def extract_min(self):
        if self.head is None:
            return None
        # find min root
        prev_min = None
        min_node = self.head
        prev = None
        curr = self.head
        while curr.sibling:
            self.inst.comparisons += 1
            if curr.sibling.key < min_node.key:
                min_node = curr.sibling
                prev_min = curr
            curr = curr.sibling
        # remove min_node from root list
        if prev_min:
            prev_min.sibling = min_node.sibling
        else:
            self.head = min_node.sibling
        # reverse children and create new heap
        child = min_node.child
        new_head = None
        while child:
            nxt = child.sibling
            child.sibling = new_head
            child.parent = None
            new_head = child
            child = nxt
        other = BinomialHeap()
        other.head = new_head
        self._union(other)
        return min_node.key


class FibonacciHeap:
    class Node:
        def __init__(self, key):
            self.key = key
            self.degree = 0
            self.parent = None
            self.child = None
            self.left = self
            self.right = self
            self.mark = False

    def __init__(self):
        self.min = None
        self.count = 0
        self.inst = Instrumentation()

    def _less(self, a, b):
        self.inst.comparisons += 1
        return a < b

    def _merge_lists(self, a, b):
        if not a:
            return b
        if not b:
            return a
        # merge circular doubly linked lists
        a.right.left = b.left
        b.left.right = a.right
        a.right = b
        b.left = a
        return a if self._less(a.key, b.key) else b

    def insert(self, key):
        node = FibonacciHeap.Node(key)
        self.min = self._merge_lists(self.min, node)
        self.count += 1
        return node

    def extract_min(self):
        z = self.min
        if z is not None:
            if z.child:
                # add each child to root list
                children = [x for x in self._iterate(z.child)]
                for c in children:
                    c.parent = None
                    # remove marks
                    c.mark = False
                    self.min = self._merge_lists(self.min, c)
            # remove z from root list
            if z.right == z:
                self.min = None
            else:
                z.left.right = z.right
                z.right.left = z.left
                self.min = z.right
                self._consolidate()
            self.count -= 1
        return z.key if z else None

    def _iterate(self, head):
        node = head
        if not node:
            return
        while True:
            yield node
            node = node.right
            if node == head:
                break

    def _consolidate(self):
        import math
        A = [None] * (int(math.log2(self.count)) + 2 if self.count > 0 else 1)
        nodes = [w for w in self._iterate(self.min)]
        for w in nodes:
            x = w
            d = x.degree
            while A[d] is not None:
                y = A[d]
                if self._less(y.key, x.key):
                    x, y = y, x
                self._link(y, x)
                A[d] = None
                d += 1
            A[d] = x
        self.min = None
        for node in A:
            if node:
                node.left = node.right = node
                self.min = self._merge_lists(self.min, node)

    def _link(self, y, x):
        # remove y from root list
        y.left.right = y.right
        y.right.left = y.left
        y.parent = x
        y.left = y.right = y
        x.child = self._merge_lists(x.child, y)
        x.degree += 1
        y.mark = False

    def search(self, key):
        # naive linear search on all nodes
        if not self.min:
            return False
        for node in self._iterate(self.min):
            if node.key == key:
                return True
            # search children recursively
            stack = []
            if node.child:
                stack.extend(list(self._iterate(node.child)))
            while stack:
                c = stack.pop()
                if c.key == key:
                    return True
                if c.child:
                    stack.extend(list(self._iterate(c.child)))
        return False

    def traverse(self):
        # simply iterate through root list
        if not self.min:
            return
        for node in self._iterate(self.min):
            pass


# Adaptive heap that chooses a representation at first extract
# - collects stats during inserts (counts, min/max, items)
# - on first extract picks either a counting/bucket strategy (for many duplicates or small range)
#   or falls back to a binary heap. This aims to be fast on duplicate-heavy and small-range datasets.
class AdaptiveHeap:
    def __init__(self):
        self.inst = Instrumentation()
        self._items = []
        self._counts = {}
        self._min = None
        self._max = None
        self._decided = False
        self._mode = None  # 'count' or 'heap'
        # count-mode structures
        self._sorted_keys = None
        self._key_idx = 0
        # heap-mode
        self._heap = None

    def insert(self, k):
        self._items.append(k)
        # update counts and stats
        self._counts[k] = self._counts.get(k, 0) + 1
        if self._min is None or k < self._min:
            self._min = k
        if self._max is None or k > self._max:
            self._max = k

    def _decide(self):
        n = len(self._items)
        u = len(self._counts)
        rng = (self._max - self._min) if (self._min is not None and self._max is not None and isinstance(self._min, (int, float)) and isinstance(self._max, (int, float))) else None
        # Heuristic: if unique keys are much fewer than total, or numeric range small, use counting strategy
        if u <= max(100, n // 4) or (rng is not None and rng <= max(1000, n)):
            self._mode = 'count'
            # prepare sorted unique keys for fast extractions
            self._sorted_keys = sorted(self._counts.keys())
            self._key_idx = 0
        else:
            self._mode = 'heap'
            # build a wrapped heap so comparisons are counted
            from heapq import heapify
            self._heap = [ _CmpWrapper(x, self.inst) for x in self._items ]
            heapify(self._heap)
        self._decided = True

    def extract_min(self):
        if not self._decided:
            self._decide()
        if self._mode == 'count':
            # find next non-empty key
            while self._key_idx < len(self._sorted_keys):
                key = self._sorted_keys[self._key_idx]
                if self._counts.get(key, 0) > 0:
                    self._counts[key] -= 1
                    return key
                self._key_idx += 1
            return None
        else:
            from heapq import heappop
            w = heappop(self._heap) if self._heap else None
            return w.key if w else None

    def search(self, k):
        # naive
        if not self._decided:
            return k in self._counts
        if self._mode == 'count':
            return self._counts.get(k, 0) > 0
        else:
            for w in self._heap:
                if w.key == k:
                    return True
            return False

    def traverse(self):
        # noop
        pass


class BTree:
    class BTreeNode:
        def __init__(self, t, leaf=False):
            self.t = t
            self.leaf = leaf
            self.keys = []
            self.children = []

    def __init__(self, t=3):
        self.t = t
        self.root = self.BTreeNode(t, leaf=True)
        self.inst = Instrumentation()

    def search(self, k, node=None):
        node = node or self.root
        i = 0
        while i < len(node.keys) and k > node.keys[i]:
            self.inst.comparisons += 1
            i += 1
        if i < len(node.keys) and k == node.keys[i]:
            self.inst.comparisons += 1
            return True
        if node.leaf:
            return False
        return self.search(k, node.children[i])

    def insert(self, k):
        root = self.root
        if len(root.keys) == 2 * self.t - 1:
            new_root = self.BTreeNode(self.t, leaf=False)
            new_root.children.append(root)
            self._split_child(new_root, 0)
            self.root = new_root
            self._insert_nonfull(self.root, k)
        else:
            self._insert_nonfull(root, k)

    def _split_child(self, parent, i):
        t = self.t
        y = parent.children[i]
        z = self.BTreeNode(t, leaf=y.leaf)
        z.keys = y.keys[t:]
        y.keys = y.keys[:t-1]
        if not y.leaf:
            z.children = y.children[t:]
            y.children = y.children[:t]
        parent.children.insert(i+1, z)
        parent.keys.insert(i, y.keys.pop())
        self.inst.rotations += 1

    def _insert_nonfull(self, node, k):
        i = len(node.keys) - 1
        if node.leaf:
            node.keys.append(None)
            while i >= 0 and k < node.keys[i]:
                self.inst.comparisons += 1
                node.keys[i+1] = node.keys[i]
                i -= 1
            node.keys[i+1] = k
        else:
            while i >= 0 and k < node.keys[i]:
                self.inst.comparisons += 1
                i -= 1
            i += 1
            if len(node.children[i].keys) == 2*self.t - 1:
                self._split_child(node, i)
                if k > node.keys[i]:
                    i += 1
            self._insert_nonfull(node.children[i], k)

    def delete(self, k):
        if self.search(k):
            pass
        return False

    def traverse(self, node=None):
        node = node or self.root
        i = 0
        for i in range(len(node.keys)):
            if not node.leaf:
                self.traverse(node.children[i])
        if not node.leaf:
            self.traverse(node.children[i+1])


class RBNode:
    def __init__(self, k, color='red'):
        self.k = k
        self.color = color
        self.left = None
        self.right = None
        self.parent = None


class RedBlackTree:
    def __init__(self):
        self.nil = RBNode(None, 'black')  # sentinel
        self.root = self.nil
        self.inst = Instrumentation()

    def insert(self, k):
        node = RBNode(k, 'red')
        node.left = self.nil
        node.right = self.nil
        node.parent = None
        
        # binary search tree insertion
        y = None
        x = self.root
        while x != self.nil:
            self.inst.comparisons += 1
            y = x
            if k < x.k:
                x = x.left
            else:
                x = x.right
        
        node.parent = y
        if y is None:
            self.root = node
        elif k < y.k:
            y.left = node
        else:
            y.right = node
        
        # fix violations
        self._fix_insert(node)

    def _fix_insert(self, node):
        while node.parent and node.parent.color == 'red':
            self.inst.rotations += 1
            if node.parent == node.parent.parent.left:
                uncle = node.parent.parent.right
                if uncle.color == 'red':
                    node.parent.color = 'black'
                    uncle.color = 'black'
                    node.parent.parent.color = 'red'
                    node = node.parent.parent
                else:
                    if node == node.parent.right:
                        node = node.parent
                        self._rotate_left(node)
                    node.parent.color = 'black'
                    node.parent.parent.color = 'red'
                    self._rotate_right(node.parent.parent)
            else:
                uncle = node.parent.parent.left
                if uncle.color == 'red':
                    node.parent.color = 'black'
                    uncle.color = 'black'
                    node.parent.parent.color = 'red'
                    node = node.parent.parent
                else:
                    if node == node.parent.left:
                        node = node.parent
                        self._rotate_right(node)
                    node.parent.color = 'black'
                    node.parent.parent.color = 'red'
                    self._rotate_left(node.parent.parent)
        self.root.color = 'black'

    def _rotate_left(self, node):
        right_child = node.right
        node.right = right_child.left
        if right_child.left != self.nil:
            right_child.left.parent = node
        right_child.parent = node.parent
        if node.parent is None:
            self.root = right_child
        elif node == node.parent.left:
            node.parent.left = right_child
        else:
            node.parent.right = right_child
        right_child.left = node
        node.parent = right_child

    def _rotate_right(self, node):
        left_child = node.left
        node.left = left_child.right
        if left_child.right != self.nil:
            left_child.right.parent = node
        left_child.parent = node.parent
        if node.parent is None:
            self.root = left_child
        elif node == node.parent.left:
            node.parent.left = left_child
        else:
            node.parent.right = left_child
        left_child.right = node
        node.parent = left_child

    def search(self, k):
        node = self.root
        while node != self.nil:
            self.inst.comparisons += 1
            if k == node.k:
                return True
            elif k < node.k:
                node = node.left
            else:
                node = node.right
        return False

    def delete(self, k):
        return False  # not implemented

    def traverse(self):
        def _inorder(node):
            if node != self.nil:
                _inorder(node.left)
                _inorder(node.right)
        _inorder(self.root)

    def height(self):
        def _h(node):
            if node == self.nil:
                return 0
            return 1 + max(_h(node.left), _h(node.right))
        return _h(self.root)
