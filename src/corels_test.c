#include "rule.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "wpa_objective.h"

int main(int argc, char *argv[])
{
    int wpa = 0;
    int e = 1;
    int c;
    while ((c = getopt(argc, argv, "we:")) != -1) {
        switch (c) {
            case 'w':
                wpa = 1;
                break;
            case 'e':
                e = atoi(optarg);
                break;
        }
    }
    
    int nrules, nsamples, nlabels, nsamples_chk;
    rule_t *rules, *labels;
    rules_init(argv[optind], &nrules, &nsamples, &rules, 0);
    rules_init(argv[optind+1], &nlabels, &nsamples_chk, &labels, 0);
    int rulelist_cnt = argc - optind - 2;
    int ruleindex[rulelist_cnt];
    char **rulelist = &argv[optind+2];

    // Find the rules in the rule list
    for (int i = 0; i < rulelist_cnt; i++) {
        for (int j = 0; j < nrules; j++) {
            if (strcmp(rulelist[i], rules[j].features) == 0)
                ruleindex[i] = j;
        }
    }

    // Calculate scores
    double scores[nsamples];
    for (int i = 0; i < nsamples; i++)
        scores[i] = 0;
    VECTOR captured, total_captured, ones;
    rule_vinit(nsamples, &captured);
    rule_vinit(nsamples, &total_captured);
    rule_vinit(nsamples, &ones);
    int ncaptured, total_ncaptured, num_ones;
    double proportion;
    rule_t ones_label = labels[1]; // Make copy to remove const
    for (int i = 0; i < rulelist_cnt; i++) {
        rule_t curr_rule = rules[ruleindex[i]];
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
    update_scores(scores, nsamples, captured, wpa, rulelist_cnt, proportion);

    // Make copy of labels
    int labels_int[nsamples];
    for (int i = 0; i < nsamples; i++)
        labels_int[i] = (rule_isset(labels[1].truthtable, nsamples-i-1) != 0);

    // Calculate objective
    int wpa_max = labels[0].support * labels[1].support;
    for (int i = 0; i < e - 1; i++)
        wpa_max *= labels[0].support;

    double wpa_obj = wpa_objective(scores, labels_int, wpa_max, nsamples, e);
    printf("%f\n", wpa_obj);

    // Print scores
    for (int i = 0; i < nsamples; i++)
        printf("%f ", scores[i]);
}