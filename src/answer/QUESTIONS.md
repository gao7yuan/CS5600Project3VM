NOTES FOR GRADING:
I only added some logs to `stack/singleThreadedTest.c` and did not change other parts of the test.

1. Describe your design of the page table and frame tables. How and why did you store your data the way you did?
- Page table
  - Page table is an array of page table entries (`PageTableEntry`). The array size is 2k which is equal to the total number of pages (`NUM_PAGES`). The array indices represent the virtual page number (VPN). It basically acts as a hash table that maps VPN to physical frame number (PFN).
  - `PageTableEntry` contains two fields. One is `int physicalFrameNumber`, which is the PFN that this page matches to. The other is `int present`, which represents whether this page is present on the physical frame or not.
  - This makes it efficient to find the corresponding frame number of a virtual page, and whether the page is present on the physical frame or cached to disk.
- Frame table
  - `FrameTable` is a struct that contains the following attributes.
  - An array of `FrameTableEntry` with size of 2k (`NUM_PAGES`). The array indices represent PFN, and it is essentially a hash table too, which maps PFN to (thread, VPN).
    - In each `FrameTableEntry`, there is `int threadId` and `Thread *thread` so that we know which thread is currently related to this physical frame.
    - There is also a `virtualFrameNumber`, which maps current frame to a virtual page of the current thread.
    - The `int accessed` attribute in `FrameTableEntry` tells us whether this frame has recently been accessed or not, for the sake of clock algorithm.
    - Each frame also has a lock `pthread_mutex_t lock`. When we read from or write to a certain frame we need to make sure it's thread safe.
  - `int index` which indicates current starting index for implementation of clock algorithm.
  - `pthread_mutex_t lock`, a lock for the whole frame table to deal with concurrency.
2. Describe your algorithm for finding the frame associated with a page.
- Given the virtual page number VPN, we follow the following steps to look for the physical frame number PFN associated with it:
  1. From the page table, we use VPN as the array index to find the `PageTableEntry` we are interested in.
  2.
  - The PFN of this `PageTableEntry` would be -1 by default if we have not done any translation yet to find the matching PFN. In this case, we scan the whole frame table to find an available frame.
    - If there is one, we return PFN of this frame.
    - Otherwise, we need to apply clock algorithm and find a frame to evict, so that we can spare this frame for this page. Basically we start from the starting index in the frame table, check whether there is a frame that has not been accessed recently. If there is one, evict it. Otherwise, we evict the first frame. After clock algorithm is done, we return the PFN that we just swapped page.
  - If PFN is not -1, it means that there is already a frame assigned to this page. We continue to check the `present` attribute of the `PageTableEntry`.
    - If it is 1, it means this page is present on the physical frame, we simply return its PFN.
    - If it is 0, it means this page was swapped out and cached to disk. In this case, we need to again apply clock algorithm to evict a frame so that we can swap this page back to the physical memory.
3. When two user processes both need a new frame at the same time, how are races avoided?
- We add a lock to the whole frame table when one user is scanning the frame table for available frame.
- When the user is checking the status of one frame, we also need to add a lock to the specific frame, so that no other users can access/read/write this frame.
- If this user does not need this frame any more, we unlock it. If this user decides to use this frame for further jobs, the lock is held until the user's done with reading from or writing to it.
- When the user is done scanning the whole frame table, the lock to the whole table is released.
4. Why did you choose the data structure(s) that you did for representing virtual-to-physical mappings?
- The idea is hashing would make it efficient to map VPN to PFN, and array is one possible implementation of hash table. The array index represents the VPN which ranges from 0 to 2k, and its content contains PFN.
5. When a frame is required but none is free, some frame must be evicted.  Describe your code for choosing a frame to evict.
- We start from the starting index of the frame table. For each frame table entry, we check whether it has been accessed recently. If there is one that's not been accessed, we decide to evict this frame. If after one scan of the whole table, all of them has been accessed, we simply decide to evict the first page.
6. When a process P obtains a frame that was previously used by a process Q, how do you adjust the page table (and any other data structures) to reflect the frame Q no longer has?
- Each `Thread` (or process, as in this question) has a page table that maps VPN to PFN. The system has one frame table that matches PFN to (thread, VPN).
- In the frame table, find the `FrameTableEntry` corresponding to this frame of page swapping. Find the thread (or process, which should be Q in this case) and page (say `pageQ`) that matches with it. Update `PageTableEntry` with `pageQ` for process Q in its page table. Specifically, change `present` from 1 to 0 to indicate that it's not on the frame any more.
- Update page table of process P to indicate that a certain page (`pageP`) matches this frame. Also change `present` to 1 to indicate that it's present on the frame.
- Also update frame table, to indicate that this frame now matches with (process P, `pageP`).
7. A page fault in process P can cause another process Q's frame to be evicted.  How do you ensure that Q cannot access or modify the page during the eviction process?  How do you avoid a race between P evicting Q's frame and Q faulting the page back in?
- When a process is evicting a frame, we add a lock to the frame. Other processes trying to access this frame need to wait for the lock to be available before they can process it.
8. Suppose a page fault in process P causes a page to be read from the file system or swap.  How do you ensure that a second process Q cannot interfere by e.g. attempting to evict the frame while it is still being read in?
- When a process is reading from a frame, we add a lock to the frame. Therefore, the second process would not be able to interfere until the first process is done reading from it and release the lock.
9. A single lock for the whole VM system would make synchronization easy, but limit parallelism.  On the other hand, using many locks complicates synchronization and raises the possibility for deadlock but allows for high parallelism.  Explain where your design falls along this continuum and why you chose to design it this way.
- My design used both a "global" lock for the whole frame table and "local" locks for single frames.
- When scanning the whole frame table for translatint VPN to PFN, or to swap pages, we cannot avoid using a single "global" lock for the whole frame table. Therefore, for these procedures, a single lock is added to the whole frame table.
- However, when we find the actual frame we want to read from or write to, we only need to lock this frame so that accessing other frames would not be blocked by this single process. Therefore, when reading from and writing to a frame, a "local" lock is added to the specific frame, to allow for parallelism and efficiency.
