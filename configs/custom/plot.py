import matplotlib.pyplot as plt


def plot_graph_from_file():
    exp_dir_name = "./data/"
    traffics = ["uniform_random"]
    suffix_op = ["_deter.txt", "_sta_1.txt", "_dyn_0_1.txt"]
    labelpost = ["Deterministic", "SA", "DA"]
    linestyle = ["-", "--", "-."]
    fileprefix = "k8d2_"
    # Create a plot
    plt.figure(figsize=(10, 6))
    # plt.ylim(0, 20000)
    for traffic_name in traffics:
        for op in range(len(suffix_op)):
            with open(
                exp_dir_name + fileprefix + traffic_name + suffix_op[op], "r"
            ) as file:
                lines = file.readlines()
            line_no = 0
            injection_rate = []
            throughput = []
            packets_inj = []
            packets_rec = []
            avg_queue_lat = []
            avg_network_lat = []
            avg_lat = []
            avg_hop = []
            avg_drs = []
            avg_misrouting = []
            reception_rate = []
            # hyperparam = int(lines[line_no])
            # line_no += 1
            print(len(lines))
            while line_no in range(len(lines)):
                injection_rate_p = float(lines[line_no])
                packets_inj_p = float(lines[line_no + 1])
                packets_rec_p = float(lines[line_no + 2])
                avg_queue_lat_p = float(lines[line_no + 3]) / 50
                avg_network_lat_p = float(lines[line_no + 4]) / 50
                avg_lat_p = float(lines[line_no + 5]) / 50
                avg_hop_p = float(lines[line_no + 6])
                avg_drs_p = float(lines[line_no + 7])
                avg_misrouting_p = float(lines[line_no + 8])
                reception_rate_p = float(lines[line_no + 9])
                line_no += 10
                injection_rate.append(injection_rate_p)
                throughput.append(packets_rec_p / 50)
                packets_inj.append(packets_inj_p)
                packets_rec.append(packets_rec_p)
                avg_queue_lat.append(avg_queue_lat_p)
                avg_network_lat.append(avg_network_lat_p)
                avg_lat.append(avg_lat_p)
                avg_hop.append(avg_hop_p)
                avg_drs.append(avg_drs_p)
                avg_misrouting.append(avg_misrouting_p)
                reception_rate.append(reception_rate_p)
            if len(avg_lat) > 0:
                plt.plot(
                    injection_rate,
                    avg_network_lat,
                    linestyle=linestyle[op],
                    label=traffic_name + "(" + labelpost[op] + ")",
                )
            # plt.plot(injection_rate, avg_queue_lat, marker='o', linestyle='-', color='g')
            # plt.plot(injection_rate, avg_network_lat, marker='o', linestyle='-', color='b')

    # Set title and labels
    # plt.title(title)
    plt.xlabel("Injection Rate (Packets/Node/Cycle)")
    plt.ylabel("Throughput (Packets/Cycle)")
    plt.legend()

    # Show the plot
    plt.grid(True)
    plt.show()
    # plt.savefig('../../../lab3/hop.png')


if __name__ == "__main__":
    # Specify the file name and title for the plot
    plot_graph_from_file()
