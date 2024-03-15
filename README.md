# ib_monitor

InfiniBand interface trafic monitor, tested on DGX/HGX A100/H100/A800/H800 , DGXOS/CentOS/Ubuntu

## System requirements

c++ 11

## Usage

1. clone this repo
2. compile with `g++ -std=c++11 ib_monitor.cpp -o ib_monitor -lpthread`
3. run `./ib_monitor`

## Output

![ib monitor](https://github.com/kooqi/ib_monitor/blob/main/image/ib_monitor.png?raw=true "ib monitor")

## DGX port map

![DGX H100 ](https://github.com/kooqi/ib_monitor/blob/main/image/A100.png?raw=true "DGX H100")
![port map ](https://github.com/kooqi/ib_monitor/blob/main/image/map.png?raw=true "port map")

> port_xmit_data: (RO) Total number of data octets, divided by 4 (lanes), transmitted on all VLs. This is 64 bit counter
port_rcv_data: (RO) Total number of data octets, divided by 4 (lanes), received on all VLs. This is 64 bit counter.
> From:  Documentation/ABI/stable/sysfs-class-infiniband

``` c++
pma_cnt_ext->port_xmit_data =
    cpu_to_be64(MLX5_SUM_CNT(out, transmitted_ib_unicast.octets,
                 transmitted_ib_multicast.octets) >> 2);
pma_cnt_ext->port_rcv_data =
    cpu_to_be64(MLX5_SUM_CNT(out, received_ib_unicast.octets,
                 received_ib_multicast.octets) >> 2);
```

> file: drivers/infiniband/hw/mlx5/mad.c
