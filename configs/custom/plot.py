import matplotlib.pyplot as plt

data_dir_path = "./data/"
graph_dir_path = "./graph/"


def args_to_file_name(k, d, traffic_pattern, routing_algo, degree, picking_algo):
    prefix = "k" + str(k) + "d" + str(d) + "_" + traffic_pattern
    if routing_algo == 0:
        return prefix + "_deter.txt"
    elif routing_algo == 1:
        return prefix + "_sta_" + str(degree) + ".txt"
    else:
        return prefix + "_dyn_" + str(degree) + "_" + str(picking_algo) + ".txt"


def axis_name(op):
    axisname = ["injrate",
                "injpac",
                "recpac",
                "qlat",
                "nlat",
                "lat",
                "hop",
                "dr",
                "misroute",
                "recrate"]
    return axisname[op]


def axis_label(op):
    axislabel = ["Injection Rate (Packets/Cycle/Node)",
                 "Packets Injected (Packets)",
                 "Packets Received (Packets)",
                 "Average Queuing Latency (Cycle)",
                 "Average Network Latency (Cycle)",
                 "Average Latency (Cycle)",
                 "Average Hops",
                 "Average Dimension Reversals",
                 "Average Misrouting Times",
                 "Reception Rate (Packets/Cycle/Node)"]
    return axislabel[op]


def plot_graph_from_file(filenames, labels, linecolors, x_axis, y_axis, savename):
    # Create a plot
    plt.figure(figsize=(10, 6))
    # plt.ylim(0, 20000)
    for op in range(len(filenames)):
        with open(
                data_dir_path + filenames[op], "r"
        ) as file:
            lines = file.readlines()
        line_no = 0
        x_data = []
        y_data = []
        # hyperparam = int(lines[line_no])
        # line_no += 1
        while line_no in range(len(lines)):
            xp = float(lines[line_no + x_axis])
            yp = float(lines[line_no + y_axis])
            if 3 <= x_axis <= 5:
                xp /= 50
            if 3 <= y_axis <= 5:
                yp /= 50
            line_no += 10
            x_data.append(xp)
            y_data.append(yp)
        if len(x_data) > 0:
            plt.plot(
                x_data,
                y_data,
                color=linecolors[op],
                label=labels[op],
            )

    # Set title and labels
    # plt.title(title)
    plt.xlabel(axis_label(x_axis))
    plt.ylabel(axis_label(y_axis))
    plt.legend()

    # Show the plot
    plt.grid(True)
    if savename == "":
        plt.show()
    else:
        plt.savefig(graph_dir_path + savename + ".png")


def compare_algo(traffic_pattern, x_axis, y_axis, save):
    filenames = [
        args_to_file_name(8, 2, traffic_pattern, 0, -1, -1),
        args_to_file_name(8, 2, traffic_pattern, 1, 1, -1),
        args_to_file_name(8, 2, traffic_pattern, 2, 0, 1)]
    labels = ["Deterministic", "Static Adaptive", "Dynamic Adaptive"]
    colors = ["r", "g", "b"]
    if save:
        plot_graph_from_file(filenames, labels, colors, x_axis, y_axis, traffic_pattern + "_" + axis_name(y_axis))
    else:
        plot_graph_from_file(filenames, labels, colors, x_axis, y_axis, "")


def compare_sta(traffic_pattern, x_axis, y_axis, save):
    filenames = [
        args_to_file_name(8, 2, traffic_pattern, 0, -1, -1),
        args_to_file_name(8, 2, traffic_pattern, 1, 1, -1),
        args_to_file_name(8, 2, traffic_pattern, 1, 2, -1)]
    labels = ["Deterministic", "DR <= 1", "DR <= 2"]
    colors = ["r", "g", "b"]
    if save:
        plot_graph_from_file(filenames, labels, colors, x_axis, y_axis, traffic_pattern + "_" + axis_name(y_axis))
    else:
        plot_graph_from_file(filenames, labels, colors, x_axis, y_axis, "")


def compare_throttling(traffic_pattern, x_axis, y_axis, save):
    filenames = [
        args_to_file_name(8, 2, traffic_pattern, 2, 0, 1),
        args_to_file_name(8, 2, traffic_pattern, 2, 2, 1),
        args_to_file_name(8, 2, traffic_pattern, 2, 4, 1)]
    labels = ["No Throttling", "2 vcs", "4 vcs"]
    colors = ["r", "g", "b"]
    if save:
        plot_graph_from_file(filenames, labels, colors, x_axis, y_axis, traffic_pattern + "_" + axis_name(y_axis))
    else:
        plot_graph_from_file(filenames, labels, colors, x_axis, y_axis, "")


def compare_pick(traffic_pattern, x_axis, y_axis, save):
    filenames = [
        args_to_file_name(8, 2, traffic_pattern, 2, 0, 0),
        args_to_file_name(8, 2, traffic_pattern, 2, 0, 1),
        args_to_file_name(8, 2, traffic_pattern, 2, 0, 2)]
    labels = ["Random", "Minimum Congestion", "Straight Lines"]
    colors = ["r", "g", "b"]
    if save:
        plot_graph_from_file(filenames, labels, colors, x_axis, y_axis, traffic_pattern + "_" + axis_name(y_axis))
    else:
        plot_graph_from_file(filenames, labels, colors, x_axis, y_axis, "")


if __name__ == "__main__":
    compare_algo("bit_complement", 0, 9, 0)
