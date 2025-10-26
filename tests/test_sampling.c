#include "test_framework.h"
#include "../models/text/generate.h"
#include "../models/text/sampling.h"

#include <string.h>

static void initParams(HyperionGenerationParams *params)
{
    memset(params, 0, sizeof(*params));
    params->maxTokens      = 16;
    params->temperature    = 1.0f;
    params->samplingMethod = HYPERION_SAMPLING_GREEDY;
    params->topK           = 0;
    params->topP           = 0.0f;
}

HYPERION_TEST(test_sampling_topk_zero_behaves_greedy)
{
    float logits[4] = {5.0f, 2.0f, 1.0f, -3.0f};

    HyperionGenerationParams params;
    initParams(&params);
    params.samplingMethod = HYPERION_SAMPLING_TOP_K;
    params.topK           = 0;

    hyperionSamplingSeedRandom(42);

    int token = hyperionSampleToken(logits, 4, &params);

    HYPERION_ASSERT(token == 0, "topK=0 should return highest-probability token");

    return 0;
}

HYPERION_TEST(test_sampling_topk_respects_subset)
{
    float logits[3] = {4.0f, 3.5f, -10.0f};

    HyperionGenerationParams params;
    initParams(&params);
    params.samplingMethod = HYPERION_SAMPLING_TOP_K;
    params.topK           = 2;

    hyperionSamplingSeedRandom(1337);

    int token = hyperionSampleToken(logits, 3, &params);

    HYPERION_ASSERT(token == 0 || token == 1,
                    "topK sample should remain within highest-probability set");

    return 0;
}

HYPERION_TEST(test_sampling_topp_prefers_highest_mass)
{
    float logits[3] = {6.0f, 1.0f, -6.0f};

    HyperionGenerationParams params;
    initParams(&params);
    params.samplingMethod = HYPERION_SAMPLING_TOP_P;
    params.topP           = 0.7f;

    hyperionSamplingSeedRandom(7);

    int token = hyperionSampleToken(logits, 3, &params);

    HYPERION_ASSERT(token == 0, "topP should select the dominant probability mass");

    return 0;
}

const HyperionTestCase g_sampling_tests[] = {
    {"sampling_topk_zero_behaves_greedy", "text", test_sampling_topk_zero_behaves_greedy},
    {"sampling_topk_respects_subset", "text", test_sampling_topk_respects_subset},
    {"sampling_topp_prefers_highest_mass", "text", test_sampling_topp_prefers_highest_mass},
};

const size_t g_sampling_test_count = sizeof(g_sampling_tests) / sizeof(g_sampling_tests[0]);
