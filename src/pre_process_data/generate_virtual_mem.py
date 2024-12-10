import random
import matplotlib
matplotlib.use('Agg')  # Use non-GUI backend to avoid Qt errors
import matplotlib.pyplot as plt

def generate_memory_access_data_with_log(output_file, num_accesses, locality_ratio=70, address_space=0x15211e2a, num_clusters=10):
    """
    Generate memory access data with multiple clusters and plot both linear and log scale distributions.
    """
    #page_size = 0x1000  # 4KB pages
    #page_size = 0x4000  # 16kb
    page_size = 0x400   # 1kB
    #page_size = 0x10000 # 64kB 

    accesses = []

    # Generate multiple base addresses for clusters
    base_addresses = [random.randint(0, address_space // page_size) * page_size for _ in range(num_clusters)]
    recent_addresses_per_cluster = {base: [base + i * 4 for i in range(8)] for base in base_addresses}

    for _ in range(num_accesses):
        recent_addresses = None  # Initialize recent_addresses

        if random.randint(0, 100) < locality_ratio:
            # Choose a random cluster for this access
            cluster_base = random.choice(base_addresses)
            recent_addresses = recent_addresses_per_cluster[cluster_base]

            # Locality: Temporal or Spatial
            if random.randint(0, 1) == 0:
                # Temporal locality: Reuse recent addresses within the cluster
                va = random.choice(recent_addresses)
            else:
                # Spatial locality: Incremental address in the same page or nearby page within the cluster
                va = cluster_base + random.randint(0, page_size - 1)
        else:
            # Random access
            va = random.randint(0, address_space)

        # Ensure the virtual address is within the address space
        va = va & address_space

        # Choose read or write randomly (0 = READ, 1 = WRITE)
        action = random.randint(0, 1)

        # Generate a value for WRITE operations
        if action == 1:
            value = random.choice("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789")
        else:
            value = None

        # Add to access list
        accesses.append((va, action, value))

        # Update recent addresses for temporal locality (only if recent_addresses is valid)
        if recent_addresses is not None and action == 1:  # Update only for writes
            recent_addresses.append(va)
            if len(recent_addresses) > 8:
                recent_addresses.pop(0)  # Maintain a small history

    # Write data to the output file
    with open(output_file, "w") as f:
        for va, action, value in accesses:
            if action == 0:  # READ
                f.write(f"{va:#010x} {action}\n")
            else:  # WRITE
                f.write(f"{va:#010x} {action} {value}\n")

    # #Plot the linear scale distribution
    # plot_access_distribution(accesses, address_space)
    # # Plot the log scale distribution
    # plot_access_distribution_log_scale(accesses, address_space)

# def plot_access_distribution(accesses, address_space=0x15211e2a):
#     """
#     Plot the distribution of memory accesses on a linear scale.
#     """
#     addresses = [va for va, _, _ in accesses]
#     normalized_addresses = [va / address_space for va in addresses]

#     plt.figure(figsize=(12, 6))
#     plt.hist(normalized_addresses, bins=100, color="skyblue", edgecolor="black", alpha=0.7)
#     plt.title("Memory Access Distribution (Linear Scale)", fontsize=16)
#     plt.xlabel("Normalized Address Space", fontsize=12)
#     plt.ylabel("Access Frequency", fontsize=12)
#     plt.grid(axis="y", alpha=0.75)
#     plt.savefig("linear_distribution.png")  # Save the plot to a file

# def plot_access_distribution_log_scale(accesses, address_space=0x15211e2a):
#     """
#     Plot the distribution of memory accesses on a log scale.
#     """
#     addresses = [va for va, _, _ in accesses]
#     normalized_addresses = [va / address_space for va in addresses]

#     plt.figure(figsize=(12, 6))
#     plt.hist(normalized_addresses, bins=100, color="skyblue", edgecolor="black", alpha=0.7, log=True)
#     plt.title("Memory Access Distribution (Log Scale)", fontsize=16)
#     plt.xlabel("Normalized Address Space", fontsize=12)
#     plt.ylabel("Log Scale of Access Frequency", fontsize=12)
#     plt.grid(axis="y", alpha=0.75)
#     #plt.savefig("log_distribution.png")  # Save the plot to a file

# Example usage
max_virtual_address = 354491946  # 0x15211e2a
num_accesses = 100000  # Number of memory accesses to generate

for i in range(1,11):
    output_file = f"data_{i}.txt"

    generate_memory_access_data_with_log(
        output_file=output_file,
        num_accesses=num_accesses,
        locality_ratio=80,
        address_space=max_virtual_address,
        num_clusters=50
    )
