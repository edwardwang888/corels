#include "queue.hh"
#include <iostream>
#include <stdio.h>
#include <getopt.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "wpa_objective.h"

#define BUFSZ 512

/*
 * Logs statistics about the execution of the algorithm and dumps it to a file.
 * To turn off, pass verbosity <= 1
 */
NullLogger* logger;

int main(int argc, char *argv[]) {
    const char usage[] = "USAGE: %s [-b] "
        "[-n max_num_nodes] [-r regularization] [-v verbosity] "
        "-c (1|2|3|4) -p (0|1|2) [-f logging_frequency]"
        "-a (0|1|2) [-s] [-L latex_out]"
        "data.out data.label\n\n"
        "%s\n";

    extern char *optarg;
    bool run_bfs = false;
    bool run_curiosity = false;
    int curiosity_policy = 0;
    bool latex_out = false;
    bool use_prefix_perm_map = false;
    bool use_captured_sym_map = false;
    int verbosity = 0;
    int map_type = 0;
    int max_num_nodes = 100000;
    double c = 0.01;
    char ch;
    bool error = false;
    char error_txt[BUFSZ];
    int freq = 1000;
    int ablation = 0;
    bool calculate_size = false;
    char verbstr[BUFSZ]; 
    bool falling = false;
    bool show_proportion = false;
    bool wpa = false;
    bool override_obj = false;
    int outfd = 0;
    char *outfile;
    double ties = 0;
    double random = 1;
    double bound = 1;
    /* only parsing happens here */
    while ((ch = getopt(argc, argv, "bsLdewWc:p:v:n:r:f:a:o:t:R:B:")) != -1) {
        switch (ch) {
        case 'b':
            run_bfs = true;
            break;
        case 's':
            calculate_size = true;
            break;
        case 'c':
            run_curiosity = true;
            curiosity_policy = atoi(optarg);
            break;
        case 'L':
            latex_out = true;
            break;
        case 'p':
            map_type = atoi(optarg);
            use_prefix_perm_map = map_type == 1;
            use_captured_sym_map = map_type == 2;
            break;
        case 'v':
            verbosity = atoi(optarg);
            break;
        case 'n':
            max_num_nodes = atoi(optarg);
            break;
        case 'r':
            c = atof(optarg);
            break;
        case 'f':
            freq = atoi(optarg);
            break;
        case 'a':
            ablation = atoi(optarg);
            break;
        case 'd':
            falling = true;
            break;
        case 'e':
            show_proportion = true;
            break;
        case 'o':
        {
            bool newfile = false;
            outfile = optarg;
            if (access(optarg, F_OK) != 0)
                newfile = true;
            outfd = open(optarg, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
            if (outfd == -1) {
                fprintf(stderr, "Error opening file: %s\n", optarg);
                exit(1);
            }
            if (newfile) {
                char text[500] = "Regularity,Length,Accuracy,WPA_Obj,Tree_Obj,Error\n";
                if (write(outfd, text, strlen(text)) == -1)
                    printf("Error in writing to %s: %s\n", outfile, strerror(errno));
            }
            break;
        }
        case 'w':
            wpa = true;
            break;
        case 'W':
            override_obj = true;
            break;
        case 't':
            ties = atof(optarg);
            break;
        case 'R':
            random = atof(optarg);
            break;
        case 'B':
            bound = atof(optarg);
            break;
        default:
            error = true;
            snprintf(error_txt, BUFSZ, "unknown option: %c", ch);
        }
    }
    if (max_num_nodes < 0) {
        error = true;
        snprintf(error_txt, BUFSZ, "number of nodes must be positive");
    }
    if (c < 0) {
        error = true;
        snprintf(error_txt, BUFSZ, "regularization constant must be postitive");
    }
    if (map_type > 2 || map_type < 0) {
        error = true;
        snprintf(error_txt, BUFSZ, "symmetry-aware map must be (0|1|2)");
    }
    if ((run_bfs + run_curiosity) != 1) {
        error = true;
        snprintf(error_txt, BUFSZ,
                "you must use at least and at most one of (-b | -c)");
    }
    if (argc < 2 + optind) {
        error = true;
        snprintf(error_txt, BUFSZ,
                "you must specify data files for rules and labels");
    }
    if (run_curiosity && !((curiosity_policy >= 1) && (curiosity_policy <= 4))) {
        error = true;
        snprintf(error_txt, BUFSZ,
                "you must specify a curiosity type (1|2|3|4)");
    }

    if (error) {
        fprintf(stderr, usage, argv[0], error_txt);
        exit(1);
    }

    std::map<int, std::string> curiosity_map;
    curiosity_map[1] = "curiosity";
    curiosity_map[2] = "curious_lb";
    curiosity_map[3] = "curious_obj";
    curiosity_map[4] = "dfs";

    argc -= optind;
    argv += optind;

    int nrules, nsamples, nlabels, nsamples_chk;
    rule_t *rules, *labels;
    rules_init(argv[0], &nrules, &nsamples, &rules, 1);
    rules_init(argv[1], &nlabels, &nsamples_chk, &labels, 0);

    int nmeta, nsamples_check;
    // Equivalent points information is precomputed, read in from file, and stored in meta
    rule_t *meta;
    if (argc == 3)
        rules_init(argv[2], &nmeta, &nsamples_check, &meta, 0);
    else
        meta = NULL;

    if (verbosity >= 10)
        print_machine_info();
    char froot[BUFSZ];
    char log_fname[BUFSZ];
    char opt_fname[BUFSZ];
    const char* pch = strrchr(argv[0], '/');
    snprintf(froot, BUFSZ, "../logs/for-%s-%s%s-%s-%s-removed=%s-max_num_nodes=%d-c=%.7f-v=%d-f=%d",
            pch ? pch + 1 : "",
            run_bfs ? "bfs" : "",
            run_curiosity ? curiosity_map[curiosity_policy].c_str() : "",
            use_prefix_perm_map ? "with_prefix_perm_map" : 
                (use_captured_sym_map ? "with_captured_symmetry_map" : "no_pmap"),
            meta ? "minor" : "no_minor",
            ablation ? ((ablation == 1) ? "support" : "lookahead") : "none",
            max_num_nodes, c, verbosity, freq);
    snprintf(log_fname, BUFSZ, "%s.txt", froot);
    snprintf(opt_fname, BUFSZ, "%s-opt.txt", froot);

    if (access(opt_fname, F_OK) == 0) {
        printf("writing optimal rule list to: %s\n\n", opt_fname);
        exit(0);
    }

    if (verbosity.count("rule")) {
        printf("\n%d rules %d samples\n\n", nrules, nsamples);
        rule_print_all(rules, nrules, nsamples);

        printf("\nLabels (%d) for %d samples\n\n", nlabels, nsamples);
        rule_print_all(labels, nlabels, nsamples);
    }

    if (verbosity > 1)
        logger = new Logger(c, nrules, verbosity, log_fname, freq);
    else
        logger = new NullLogger();
    double init = timestamp();
    char run_type[BUFSZ];
    Queue* q;
    strcpy(run_type, "LEARNING RULE LIST via ");
    char const *type = "node";
    if (curiosity_policy == 1) {
        strcat(run_type, "CURIOUS");
        q = new Queue(curious_cmp, run_type);
        type = "curious";
    } else if (curiosity_policy == 2) {
        strcat(run_type, "LOWER BOUND");
        q = new Queue(lb_cmp, run_type);
    } else if (curiosity_policy == 3) {
        strcat(run_type, "OBJECTIVE");
        q = new Queue(objective_cmp, run_type);
    } else if (curiosity_policy == 4) {
        strcat(run_type, "DFS");
        q = new Queue(dfs_cmp, run_type);
    } else {
        strcat(run_type, "BFS");
        q = new Queue(base_cmp, run_type);
    }

    PermutationMap* p;
    if (use_prefix_perm_map) {
        strcat(run_type, " Prefix Map\n");
        PrefixPermutationMap* prefix_pmap = new PrefixPermutationMap;
        p = (PermutationMap*) prefix_pmap;
    } else if (use_captured_sym_map) {
        strcat(run_type, " Captured Symmetry Map\n");
        CapturedPermutationMap* cap_pmap = new CapturedPermutationMap;
        p = (PermutationMap*) cap_pmap;
    } else {
        strcat(run_type, " No Permutation Map\n");
        NullPermutationMap* null_pmap = new NullPermutationMap;
        p = (PermutationMap*) null_pmap;
    }

    CacheTree* tree = new CacheTree(nsamples, nrules, c, rules, labels, meta, ablation, calculate_size, type, wpa);
    if (verbosity.count("progress"))
        printf("%s", run_type);
    // runs our algorithm
    bool change_search_path = outfile != NULL && strcmp(outfile, "default.csv") != 0;
    bbound(tree, max_num_nodes, q, p, falling, show_proportion, change_search_path, ties, random, bound);

    printf("final num_nodes: %zu\n", tree->num_nodes());
    printf("final num_evaluated: %zu\n", tree->num_evaluated());
    printf("final min_objective: %1.5f\n", tree->min_objective());
    const tracking_vector<unsigned short, DataStruct::Tree>& r_list = tree->opt_rulelist();

    double accuracy, scores[nsamples];
    for (int i = 0; i < nsamples; i++)
        scores[i] = 0;
    process_and_print_final_rulelist(r_list, tree->opt_predictions(),
                     latex_out, rules, labels, opt_fname, verbosity.count("progress"), wpa, nsamples, nrules, scores, c, &accuracy);
    
    // for (int i = 0; i < nsamples; i++)
    //     printf("%f ", scores[i]);
    // printf("\n");
    
    // Calculate WPA objective
    double wpa_max = labels[0].support * labels[1].support;
    double wpa_obj = 0;
    for (int i = 0; i < nsamples; i++)
        for (int j = 0; j < nsamples; j++)
            wpa_obj -= (scores[i] > scores[j]) * (rule_isset(labels[1].truthtable, i) > (rule_isset(labels[1].truthtable, j)));
    
    wpa_obj = wpa_obj/wpa_max + 1;
    
    // Override objective with WPA
    double obj_error = wpa_obj - tree->min_objective();
    // if (wpa || override_obj)
    //     tree->update_min_objective(wpa_objective);

    //double accuracy = 1 - tree->min_objective() + c*r_list.size();

    /********************************
    *** SANITY CHECK FOR OBJECTIVE **
    *********************************/
    // Make copy of labels
    int labels_int[nsamples];
    for (int i = 0; i < nsamples; i++)
        labels_int[i] = (rule_isset(labels[1].truthtable, nsamples-i-1) != 0);

    // Calculate scores
    // double scores[nsamples];
    for (int i = 0; i < nsamples; i++)
        scores[i] = 0;
    VECTOR captured, total_captured, ones;
    rule_vinit(nsamples, &captured);
    rule_vinit(nsamples, &total_captured);
    rule_vinit(nsamples, &ones);
    int ncaptured, total_ncaptured, num_ones;
    double proportion;
    rule_t ones_label = labels[1]; // Make copy to remove const
    for (int i = 0; i < r_list.size(); i++) {
        rule_t curr_rule = rules[r_list[i]];
        printf("if (%s) then (1)\n", curr_rule.features);
        rule_vandnot(captured, curr_rule.truthtable, total_captured, nsamples, &ncaptured);
        rule_vor(total_captured, captured, total_captured, nsamples, &total_ncaptured);
        // Count number of 1's
        rule_vand(ones, captured, ones_label.truthtable, nsamples, &num_ones);
        proportion = (double)num_ones/ncaptured;
        update_scores(scores, nsamples, captured, wpa, i, proportion);
    }
    /** Default rule **/
    rule_not(captured, total_captured, nsamples, &ncaptured);
    ncaptured = nsamples - total_ncaptured; // rule_not() does not handle two's complement
    // Count number of 1's
    rule_vand(ones, captured, ones_label.truthtable, nsamples, &num_ones);
    proportion = (double)num_ones/ncaptured;
    update_scores(scores, nsamples, captured, wpa, r_list.size(), proportion);
    wpa_obj = wpa_objective(scores, labels_int, wpa_max, nsamples);
    /**********************
    *** END SANITY CHECK **
    ***********************/

    if (verbosity.count("progress")) {
        printf("final num_nodes: %zu\n", tree->num_nodes());
        printf("final num_evaluated: %zu\n", tree->num_evaluated());
        printf("final min_objective: %1.5f\n", 1-tree->min_objective()/wpa_max);
        printf("final wpa_objective: %1.5f\n", wpa_obj);
        printf("final accuracy: %1.5f\n",
           accuracy);
    }

    // Output to CSV file
    if (outfd != 0) {
        char output[100];
        sprintf(output, "%1.20f,%lu,%1.20f,%1.20f,%1.20f,%1.20f\n", c, r_list.size(), accuracy, wpa_objective, tree->min_objective(), obj_error);
        if(write(outfd, output, strlen(output)) == -1)
            printf("Error in writing to %s: %s\n", outfile, strerror(errno));
        else
            printf("Writing output to: %s\n", outfile);
    }

    if (verbosity.count("progress"))
        printf("final total time: %f\n", time_diff(init));

    printf("final total time: %f\n", time_diff(init));
    logger->dumpState();
    logger->closeFile();
    if (meta) {
        printf("\ndelete identical points indicator");
        rules_free(meta, nmeta, 0);
    }
    printf("\ndelete rules\n");
    rules_free(rules, nrules, 1);
    printf("delete labels\n");
    rules_free(labels, nlabels, 0);
    printf("tree destructors\n");
    return 0;
}
