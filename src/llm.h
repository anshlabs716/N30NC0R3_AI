#ifndef LLM_H
#define LLM_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "llama.h"

/* Streaming callback: invoked for every generated token piece. */
typedef void (*llm_stream_callback)(const char *token, void *data);

/* Load the Qwen3 1.7B Q4_K_M model from ./models/. Returns 1 on success. */
int llm_init(void);

/* Free the model and context. */
void llm_cleanup(void);

/* Generate a response, optionally streaming tokens to a callback. Caller frees. */
char* llm_generate(const char *prompt, llm_stream_callback cb, void *data);

/* Blocking generation with no streaming. Caller frees. */
char* llm_generate_sync(const char *prompt);

/* 1 if a model is currently loaded, otherwise 0. */
int llm_is_loaded(void);

/* Name of the active backend: "llama.cpp", "ollama" or "none". */
const char* llm_backend_name(void);

/* Drop the rolling conversation window used for context. */
void llm_reset_history(void);

#ifdef __cplusplus
}
#endif

#endif
