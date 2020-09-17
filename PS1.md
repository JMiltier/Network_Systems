## CSCI 4273, Problem Set 1  
### Josh Miltier

**Questions** are in **bold**  
Answers in normal font  

**Note:**  
- 1 Kbps = 10<sup>3</sup> bits/sec  
- 1 Mbps = 10<sup>6</sup> bits/sec  
- 1 Gbps = 10<sup>9</sup> bits/sec  
- 1 MB = 10<sup>6</sup> x 8 bits  
- B = byte, b = bit  

----

**1. (3pts) What advantage does a circuit-switched network have over a packet-switched network?**  
  The performance of a circuit-switched network is predictable, because it's a dedicated connection/channel from point to point. This makes it very reliable with simple forwarding. Imagining when telephone companies would have switchboard operators to connect calls (though they still exist, but are automated). Very direct and straight through. No other calls could use that switch connection until the call was terminated (pre call waiting).


**2. (3pts) What advantage does TDM have over FDM in a circuit switched network?**  
  TDM - different time domains (digital)
  FDM - different frequencies over a common medium (like analog)  
  When multiple signals are being transmitted, TDM uses different time slots, preventing any crosstalk from occurring. In FDM, crosstalk can occur since the signals are over different frequency slots, but still through a common link. This leads TDM to be more efficient than FDM, mainly due to no interferences.


__3. (21pts) Consider two hosts, A and B, which are connected by a link (**R** bps). Suppose that the two hosts are separated by **m** meters, and the speed along with link is **s** meters/sec. Host A is to send a packet of size **L** bits to Host B.__  
      **a. Express the propagation delay, d<sub>prop</sub>, in terms of m and s.**  
        d<sub>prop</sub> = **m**/**s** (distance / speed; result measured in units of time)  
      **b. Determine the transmission time of the packet, d<sub>trans</sub>, in terms of L and R.**  
        d<sub>trans</sub> = **L**/**R**  (packet size / bit rate; result measured in units of time)       
      **c. Ignoring processing and queuing delays, obtain an expression for the end-to-end delay (one-way delay from Host A to Host B).**  
        One-way delay = d<sub>trans</sub> + d<sub>prop</sub> (transmission time + propagation delay; result measured in units of time)  
      **d. Suppose Host A begins to transmit the packet at time t = 0, At time t = d<sub>trans</sub>, where is the last bit of the packet?**
        Since measuring in bps: when time is equal to d<sub>trans</sub>, the last bit of the packet would be leaving from Host A.  
      **e. Suppose d<sub>prop</sub> is greater than d<sub>trans</sub>. At time t = d<sub>trans</sub>, where is the first bit of the packet?**
        The transmission time is lesser than the propagation delay, so the first bit of the packet is somewhere in transmission still.  
      **f. Suppose d<sub>prop</sub> is less than d<sub>trans</sub> . At time t = d<sub>trans</sub>, where is the first bit of the packet?**
        Contrary to the last question - since the transmission time is greater than the propagation delay, the first bit should have reached it's end host.  
      **g. Suppose s = 2.5 * 10<sup>8</sup>, L = 120bits, and R = 56Kbps. Find the distance m so that d<sub>prop</sub> equals d<sub>trans</sub>.**   
        When d<sub>prop</sub> = d<sub>trans</sub, what's the distance (m):  
        m / (2.5 * 10<sup>8</sup>m/s) = (120bits / 56Kbps)                    [move m to one side, and change units to match]  
        m = (120bits / 56 * 10<sup>3</sup>bps) * (2.5 * 10<sup>8</sup>m/s)    [subtract those powers]  
        m = (120bits / 56 bps) * (2.5 * 10<sup>5</sup>m/s)                    [probably covert the m to km]  
        m = (120bits / 56 bps) * (250km/s)     [now math]  
        m = (120bits * 250km/s) / 56 bps  
        m = 30000 b*km/s / 56bps                [bps cancels]  
        m = 535.71 km                           [if we ignore significant decimal places]  


**4. (8pts) We consider sending real-time voice from Host A to Host B over a packet-switched network. Host A converts analog voice to a digital 65kbps bit stream and send these bits into 56-byte packets. There is one link between Hosts A and B and the transmission rate is 1 Mbps and its propagation delay is 20 msec. As soon as Host A gathers a packet, it sends it to Host B. As soon as Host B receives an entire packet, it converts the packet’s bits into an analog signal. How much time elapses from the time a bit is created (from the original analog signal at Host A) until the bit is decoded (as part of the analog signal at Host B)?**  
  Data given: 65kbps (R), 56-byte packets (L), one link of (1Mbps (R), 20ms (d<sub>prop</sub>))  
  Sending over packet-switched network, once A gathers (converts from analog to digital) a packet, it sends it to B. Then B converts packets it back to analog. Time from bit created at A, until decoded at B...  
  1. So first, need to figure out how long A takes to convert a packet:  (56bytes * 8 bits/byte) / 65,000 bps = [math on calculator] = 0.006892 seconds or 6.892ms (easier to read)  
  2. Now the packet is ready, need to see how long it'll take to send from A to B: d<sub>trans</sub> = (56bytes * 8 bits/byte) / 1,000,000bps = [math] = .448ms (not bad)
  3. Time elapsed: 6.892ms + 0.448ms + 20ms (prop delay) = *27.34ms*  


**5. (12pts) Consider a Go-Back-N sliding window algorithm (1 packet is 250 bytes long) running over a 100km point-to-point fiber link with bandwidth of 100 Mbps.**   
    **1. Compute the one-way propagation delay for this link, assuming that the speed of light is 2 x 10<sup>8</sup> m/s in the fiber.**  
        d<sub>prop</sub> = 100,000m / 2 x 10<sup>8</sup> m/s = *0.5ms* (or 500μs)  
    **2. Suggest a suitable timeout value for the algorithm to use. List factors you need to consider.**  
        A good timeout value would be 1ms (double d<sub>prop</sub>), given that the link is reliable  
    **3. Suggest N to achieve 100% utilization in this link.**  
        d<sub>trans</sub> = (250 * 8 bits) / 100,000,000bps = 20μs  
        N >= ceiling[(2 * 500μs)/20] + 1 = *51 packets*


**6. (12pts) Suppose a 1-Gbps point-to-point link is being set up between the Earth and a new lunar colony. The distance from the moon to the Earth is approximately 385,000 km, and data travels over the link at the speed of light—3×10<sub>8</sub> m/s.**  
    **1. Calculate the minimum RTT for the link.**  
        One way would be 385,000km / 3×10<sub>5</sub>km/s = 1.28333 seconds (capture 6 numerical places)  
        So round time travel would be double or 2.56666 seconds.  
    **2. Using the RTT as the delay, calculate the delay × bandwidth product for the link.**  
        RTT = 2.56666s  
        2.56666s × 1Gbps = 2.56666Gb (Giga-bits, not bytes!, but to be nice, we could say 320.833MB)  
    **3. What is the significance of the delay × bandwidth product computed in (b)?**  
        This is the amount (320.833MB) of data than can be transmitted before the lunar colony's response is received back to Earth. This is assuming they send a response at the first bit, which is when half that amount of data has been sent. In other words, a stream of ~160MB from Earth is sent for 1.28333 seconds before the lunar colony gets the first bit.


**7. (12pts) Host A wants to send a 1,000 KB file to Host B. The Round Trip Time (RTT) of the Duplex Link between Host A and B is 160ms. Packet size is 1KB. A handshake between A and B is needed before data packets can start transferring which takes 2xRTT. Calculate the total required time of file transfer in the following cases. The transfer is considered complete when the acknowledgement for the final packet reaches A.**  
    Summary: A -> 1MB -> B, RTT = 160ms, L = 1KB, handshakes are 2xRTT, need to calculate time for file transfers when final packet reaches A  
    **1. The bandwidth of the link is 4Mbps. Data packets can be continuously transferred on the link.**  
        initial handshake + d<sub>trans</sub> + d<sub>prop</sub> of last packet
        (2 * 0.16sec) + (1,000bytes * 8bits / 4,000,000bps) + (0.16sec / 2)
        320ms + (1000 * 2ms) + 80ms = *2.4 seconds*   
    **2. The bandwidth of the link is 4Mbps. After sending each packet, A need to wait one RTT before the next packet can be transferred.**  
      Using equation from above, but adding in an RTT each time: 320ms + (1000 * (2ms + 160ms)) + 80ms = *162.4 seconds*   
    **3. Assume we have “unlimited” bandwidth on the link, meaning that we assume transmit time to be zero. After sending 50 packets, A need to wait one RTT before sending next group of 50 packets.**  
      Sending 1000 packets, so will need to factor in 20 RTTs (20 * 50 = 1000), no need to factor transmission time, cause it's FAST (still factor last packet)
      (160ms * 20) + 80ms = *3.28 seconds*  
    **4. The bandwidth of the link is 4Mbps. During the first transmission A can send one (2<sup>1-1</sup>) packets, during the 2<sup>nd</sup> transmission A can send 2<sup>2-1</sup> packets, during the 3rd transmission A can send 2<sup>3-1</sup> packets, and so on. Assume A still need to wait for 1 RTT between each transmission.**  
      A pattern seems to occur which is on the nth transmission, 2<sup>n-1</sup> packets can be sent. Expanding this: 1, 2, 4, 8, 16, 32, 64, 128, 256, 512 = over 1000 packets, so we're good there. We need to send 10 transmissions to cover all the packets.
      320ms + (10 * (2ms + 160ms)) + 80ms = *2.02s*


**8. (5pts) Determine the width of a bit on a 10 Gbps link. Assume a copper wire, where the speed of propagation is 2.3 ∗ 10<sup>8</sup> m/s.**   
    Wild to imagine, but here we go. Will want width per bit, so propagation speed divided by the link.  
    Going to reduce the powers and cancel the time (secs) here of (2.3 ∗ 10<sup>8</sup>m/s) / 10Gbps == 2.3m / 100b == 0.023m/b. So the width of a bit is *23mm*.  


**9. (12 pts) Suppose two hosts, A and B, are separated by 20,000 kilometers and they are connected by a direct link of R=1Gbps. Suppose the propagation speed over the link is 2.5 x 10<sup>8</sup> meters/sec.**
    **1. Calculate the bandwidth delay product (BDP) of the link**
      BDP = R * d<sub>prop</sub>, and we know R, m and s.
      BDP = 1Gbps * (20,000,000m / 2.5 x 10<sup>8</sup>m/s)   [cancel some powers again]
      BDP = 1,000,000,000bps * (2m / 25m/s)
      BDP = *80,000,000 bits*  
    **2. Consider sending a file of 800,000 bits from Host A to Host B as one large message. What is the maximum number of bits that will be in the link at any given time?**  
      From the previous question, we know that the propagation delay is 0.08 seconds.  
      So the max number of bits at any time is 1Gbps * 0.08 seconds = *80,000,000 bits*, so could really have 100 files in the link at once.  
    **3. What is the width (in meters) of a bit in the link?**
      Propagation speed = 2.5 x 10<sup>8</sup>m/s  
      R = 1Gbps  
      (2.5 x 10<sup>8</sup>m/s) / 1Gbps == 2.5m/s / 10bps = *0.25 meters/bit* in width  
    **4. Suppose now the file is broken up into 20 packets with each packet containing 40,000 bits. Suppose that each packet is acknowledged by the receiver and the transmission time of an acknowledgement packet is negligible. Finally, assume that the sender cannot send a packet until the preceding one is acknowledged. How long does it take to send the file?**
      Let's see: each packet is 40Kb, will need to determine d<sub>prop</sub>, d<sub>trans</sub>, and the fact there's now 20 packets that can't be send until the previous one was acknowledged.  
      1. d<sub>prop</sub> = 20,000,000m / 2.5 x 10<sup>8</sup>m/s = 2m / 25m/s = .08s = 80ms
      2. d<sub>trans</sub> = 40Kb / 1Gbps = 4bits / 100000bps = 0.00004s = .04ms
      3. Now, RTT is 2(80.04ms) = 160.08ms
      4. With 20 packets, 20 * 160.08ms = 3201.6ms = *3.2016 seconds* to send all of the packets


**10. (12 pts) Suppose there is a 10 Mbps microwave link between a geostationary satellite and its base station on Earth. Every minute the satellite takes a digital photo and sends it to the base station. Assume a propagation speed of 2.4 x 10<sup>8</sup> meters/sec. Geostationary satellite is 36,000 kilometers away from earth surface.**  
    **1. What is the propagation delay of the link?**  
      d<sub>prop</sub> = 36,000,000m / 2.4 x 10<sup>8</sup>m/s = 36m/240m/s = *0.15s*  
    **2. What is the bandwidth-delay product, R x (propagation delay)?**  
      BDP = 10Mbps * 0.15s = *1.5Mb* (mega-bits)  
    **3. Let *x* denote the size of the photo. What is the minimum value of *x* for the microwave link to be continuously transmitting?**  
      Photo happens every minute, at a transmission rate of 10Mbps, that means the minimum value is 60s * 10Mbps = *600Mb* (mega-bits)
