#include "rule.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void update_scores(double *scores, int nsamples, VECTOR captured, int wpa, int ruleindex, double proportion)
{
    for (int i = 0; i < nsamples; i++) {
        if (rule_isset(captured, i)) {
            if (wpa)
                *(scores+i) = -1 * ruleindex;
            else
                *(scores+i) = proportion;
        }
    }
}

int main(int argc, char *argv[])
{
    int wpa = 0;
    int c;
    while ((c = getopt(argc, argv, "w")) != -1) {
        if (c == 'w')
            wpa = 1;
    }
    
    int nrules, nsamples, nlabels, nsamples_chk;
    rule_t *rules, *labels;
    rules_init(argv[optind], &nrules, &nsamples, &rules, 0);
    rules_init(argv[optind+1], &nlabels, &nsamples_chk, &labels, 0);
    int rulelist_cnt = argc - optind - 2;
    int ruleindex[rulelist_cnt];
    char **rulelist = &argv[optind+2];
    //printf("%d ", rulelist_cnt);

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
        rule_vor(total_captured, curr_rule.truthtable, total_captured, nsamples, &total_ncaptured);
        // Count number of 1's
        rule_vand(ones, captured, ones_label.truthtable, nsamples, &num_ones);
        proportion = (double)num_ones/ncaptured;
        update_scores(scores, nsamples, captured, wpa, i, proportion);
    }
    
    // Calculate objective
    double wpa_max = labels[0].support * labels[1].support;
    double wpa_objective = 0;
    for (int i = 0; i < nsamples; i++)
        for (int j = 0; j < nsamples; j++)
            wpa_objective -= (scores[i] > scores[j]) * (rule_isset(labels[1].truthtable, i) > (rule_isset(labels[1].truthtable, j)));
    
    wpa_objective = wpa_objective/wpa_max + 1;
    printf("%f", wpa_objective);
}