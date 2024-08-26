import matplotlib.pyplot as plt


def plot_graph_from_file():

    traffics = ['uniform_random', 'shuffle', 'tornado', 'transpose']
    strop = ['_rt.txt', '_XY.txt']
    labelpost = ['Ring', 'Mesh']
    linestyle = ['-', '--']
    # Create a plot
    plt.figure(figsize=(10, 6))
   # plt.ylim(0, 20000)
    for traffic_name in traffics:
        filename_ex = 'simple_' + traffic_name
        for op in range(len(strop)):
            with open(filename_ex + strop[op], 'r') as file:
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
            reception_ratio = []
            #hyperparam = int(lines[line_no])
            #line_no += 1
            for i in range(50):
                if float(lines[line_no + 2]) < 0:
                    line_no += 6
                    continue
                injection_rate_p = float(lines[line_no])
                packets_inj_p = float(lines[line_no + 1])
                packets_rec_p = float(lines[line_no + 2])
                avg_queue_lat_p = float(lines[line_no + 3])
                avg_network_lat_p = float(lines[line_no + 4])
                avg_lat_p = float(lines[line_no + 5])
                avg_hop_p = float(lines[line_no + 6])
                line_no += 7
                injection_rate.append(injection_rate_p)
                throughput.append(packets_rec_p / 10000)
                packets_inj.append(packets_inj_p)
                packets_rec.append(packets_rec_p)
                avg_queue_lat.append(avg_queue_lat_p)
                avg_network_lat.append(avg_network_lat_p)
                avg_lat.append(avg_lat_p)
                avg_hop.append(avg_hop_p)
                reception_ratio.append(packets_rec_p / 16 / 10000 / injection_rate_p)
            if len(avg_lat) > 0:
                plt.plot(injection_rate, throughput, linestyle=linestyle[op], label=traffic_name+'('+labelpost[op]+')')
            # plt.plot(injection_rate, avg_queue_lat, marker='o', linestyle='-', color='g')
            # plt.plot(injection_rate, avg_network_lat, marker='o', linestyle='-', color='b')

    # Set title and labels
    #plt.title(title)
    plt.xlabel('Injection Rate (Packets/Node/Cycle)')
    plt.ylabel('Throughput (Packets/Cycle)')
    if len(traffics) > 1:
        plt.legend()

    # Show the plot
    plt.grid(True)
    plt.show()
    #plt.savefig('../../../lab3/hop.png')


if __name__ == "__main__":
    # Specify the file name and title for the plot
    plot_graph_from_file()
