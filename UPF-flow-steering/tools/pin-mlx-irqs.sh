#!/usr/bin/env bash
#
# -- Alex (06/03/2026)

if [ "$#" -lt 2 ]; then
    echo "Usage:"
    echo "  $0 <PCI_DEVICE> <CPU_LIST>"
    echo
    echo "Example:"
    echo "  $0 0000:31:00.0 2-23"
    exit 1
fi

PCI_DEV=$1
CPU_LIST=$2

IRQ_PATH="/sys/bus/pci/devices/$PCI_DEV/msi_irqs"

if [ ! -d "$IRQ_PATH" ]; then
    echo "No MSI IRQs found for device $PCI_DEV"
    exit 1
fi

# expand CPU list (e.g. 1-4,6 → 1 2 3 4 6)
expand_cpulist() {
    local IFS=,
    for part in $1; do
        if [[ $part == *-* ]]; then
            seq ${part%-*} ${part#*-}
        else
            echo $part
        fi
    done
}

CPUS=($(expand_cpulist "$CPU_LIST"))
CPU_COUNT=${#CPUS[@]}

echo "PCI device: $PCI_DEV"
echo "CPU list: ${CPUS[*]}"

i=0

for IRQ in $(ls "$IRQ_PATH"); do
    CPU=${CPUS[$((i % CPU_COUNT))]}

    echo "Assign IRQ $IRQ → CPU $CPU"

    echo "$CPU" > /proc/irq/$IRQ/smp_affinity_list

    ((i++))
done
