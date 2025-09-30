// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <stdint.h>

#ifdef __cplusplus
#include <cstddef>
#else
#include <stdbool.h>
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#ifdef BUILDING_ORT_GENAI_C
#define OGA_EXPORT __declspec(dllexport)
#else
#define OGA_EXPORT __declspec(dllimport)
#endif
#define OGA_API_CALL _stdcall
#else
// To make symbols visible on macOS/iOS
#ifdef __APPLE__
#define OGA_EXPORT __attribute__((visibility("default")))
#else
#define OGA_EXPORT
#endif
#define OGA_API_CALL
#endif

/** \addtogroup Global
 * ONNX Runtime Generative AI C API
 * This API is not thread safe.
 * @{
 */

typedef enum OgaElementType {
  OgaElementType_undefined,
  OgaElementType_float32,    // maps to c type float
  OgaElementType_uint8,      // maps to c type uint8_t
  OgaElementType_int8,       // maps to c type int8_t
  OgaElementType_uint16,     // maps to c type uint16_t
  OgaElementType_int16,      // maps to c type int16_t
  OgaElementType_int32,      // maps to c type int32_t
  OgaElementType_int64,      // maps to c type int64_t
  OgaElementType_string,     // string type (not currently supported by Oga)
  OgaElementType_bool,       // maps to c type bool
  OgaElementType_float16,    // IEEE 752-2008 binary16 format, 1 sign bit, 5 bit
                             // exponent, 10 bit fraction
  OgaElementType_float64,    // maps to c type double
  OgaElementType_uint32,     // maps to c type uint32_t
  OgaElementType_uint64,     // maps to c type uint64_t
  OgaElementType_complex64,  // complex with float32 real and imaginary
                             // components
  OgaElementType_complex128,  // complex with float64 real and imaginary
                              // components
  OgaElementType_bfloat16,    // Non-IEEE floating-point format based on IEEE754
                              // single-precision
} OgaElementType;

typedef struct OgaResult OgaResult;
typedef struct OgaGeneratorParams OgaGeneratorParams;
typedef struct OgaGenerator OgaGenerator;
typedef struct OgaRuntimeSettings OgaRuntimeSettings;
typedef struct OgaConfig OgaConfig;
typedef struct OgaModel OgaModel;
// OgaSequences is an array of token arrays where the number of token arrays can
// be obtained using OgaSequencesCount and the number of tokens in each token
// array can be obtained using OgaSequencesGetSequenceCount.
typedef struct OgaSequences OgaSequences;
typedef struct OgaTokenizer OgaTokenizer;
typedef struct OgaTokenizerStream OgaTokenizerStream;
typedef struct OgaTensor OgaTensor;
typedef struct OgaImages OgaImages;
typedef struct OgaNamedTensors OgaNamedTensors;
typedef struct OgaMultiModalProcessor OgaMultiModalProcessor;
typedef struct OgaAudios OgaAudios;
typedef struct OgaStringArray OgaStringArray;
typedef struct OgaAdapters OgaAdapters;
typedef struct OgaEngine OgaEngine;
typedef struct OgaRequest OgqRequest;

//! @}

/** \addtogroup Global
 * @{
 */

/**
 * \brief Call this on process exit to cleanly shutdown the genai library & its
 * onnxruntime usage
 */
OGA_EXPORT void OGA_API_CALL OgaShutdown();

/**
 * \param[in] result OgaResult that contains the error message.
 * \return Error message contained in the OgaResult. The const char* is owned by
 * the OgaResult and can will be freed when the OgaResult is destroyed.
 */
OGA_EXPORT const char* OGA_API_CALL OgaResultGetError(const OgaResult* result);

/**
 * \brief Control the logging behavior of the library.
 *        If OgaSetLogString is called with name "filename", and value is a
 * valid file path, the library will log to that file. This will override any
 * previously set logging destination. If OgaSetLogString is called with name
 * "filename" and the value provided is an empty string, the library will log to
 * the default destination (i.e. std::cerr) thereafter.
 * \param[in] name logging option name, see logging.h 'struct LogItems' for the
 * list of available options
 * \param[in] value logging option value.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaSetLogBool(const char* name, bool value);
OGA_EXPORT OgaResult* OGA_API_CALL OgaSetLogString(const char* name,
                                                   const char* value);

/**
 * \brief Register a callback function to receive log messages from the library.
 * If invoked, the callback will override the previously set logging destination
 * (e.g. a file or std::cerr).
 * \param[in] callback function pointer to the logging callback function (use
 * nullptr to disable callback and revert to the default logging destination -
 * std::cerr).
 * \return OgaResult containing the error message when the callback could not be
 * set, else nullptr.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaSetLogCallback(void (*callback)(const char* string, size_t length));

/**
 * \param[in] result OgaResult to be destroyed.
 */
OGA_EXPORT void OGA_API_CALL OgaDestroyResult(OgaResult* result);
OGA_EXPORT void OGA_API_CALL OgaDestroyString(const char*);
OGA_EXPORT void OGA_API_CALL OgaDestroyNamedTensors(OgaNamedTensors*);

OGA_EXPORT OgaResult* OGA_API_CALL OgaCreateSequences(OgaSequences** out);

/**
 * \param[in] sequences OgaSequences to be destroyed.
 */
OGA_EXPORT void OGA_API_CALL OgaDestroySequences(OgaSequences* sequences);

/**
 * \brief Returns the number of sequences in the OgaSequences
 * \param[in] sequences
 * \return The number of sequences in the OgaSequences
 */
OGA_EXPORT size_t OGA_API_CALL OgaSequencesCount(const OgaSequences* sequences);

/**
 * \brief Appends token_cnt number of tokens from token_ptr to sequence
 * \param[in] token_ptr constant pointer to int32 tokens
 * \param[in] token_cnt number of tokens to read from token_ptr
 * \param[in] sequences OgaSequences object to append the tokens to
 * \return OgaResult containing the error message when tokens could not been
 * added, else nullptr.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaAppendTokenSequence(const int32_t* token_ptr,
                       size_t token_cnt,
                       OgaSequences* sequences);

/**
 * \brief Appends the given token to the sequence at the given index.
          If the sequence at the given index does not exist, a new sequence is
          created at the given index if sequence_idx is equal to the current
 sequences count.
 * \param[in] token token to append to the sequence
 * \param[in] sequences OgaSequences object to append the token to
 * \param[in] sequence_index index of the sequence to append the token to
 * \return OgaResult containing the error message when tokens could not been
 added, else nullptr.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaAppendTokenToSequence(int32_t token,
                         OgaSequences* sequences,
                         size_t sequence_index);

/**
 * \brief Returns the number of tokens in the sequence at the given index.
 * \param[in] sequences OgaSequences to use.
 * \param[in] sequence_index index of the sequence to use.
 * \return The number of tokens in the sequence at the given index
 */
OGA_EXPORT size_t OGA_API_CALL
OgaSequencesGetSequenceCount(const OgaSequences* sequences,
                             size_t sequence_index);

/**
 * \brief Returns a pointer to the sequence data at the given index. The number
 * of tokens in the sequence is given by OgaSequencesGetSequenceCount
 * \param[in] sequences OgaSequences to use.
 * \param[in] sequence_index index of the sequence to use.
 * \return The pointer to the sequence data at the given index. The pointer is
 * valid until the OgaSequences is destroyed.
 */
OGA_EXPORT const int32_t* OGA_API_CALL
OgaSequencesGetSequenceData(const OgaSequences* sequences,
                            size_t sequence_index);

OGA_EXPORT OgaResult* OGA_API_CALL OgaLoadImage(const char* image_path,
                                                OgaImages** images);
OGA_EXPORT OgaResult* OGA_API_CALL
OgaLoadImages(const OgaStringArray* image_paths, OgaImages** images);

/**
 * \brief Load multiple images from an array of byte buffers
 * \param[in] image_data Array of byte buffers containing the image data.
 * \param[in] image_data_sizes Array of sizes of the byte buffers.
 * \param[in] count Number of images to load.
 * \param[out] images The loaded images.
 * \return OgaResult containing the error message if the loading of the images
 * failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaLoadImagesFromBuffers(const void** image_data,
                         const size_t* image_data_sizes,
                         size_t count,
                         OgaImages** images);

OGA_EXPORT void OGA_API_CALL OgaDestroyImages(OgaImages* images);

OGA_EXPORT OgaResult* OGA_API_CALL OgaLoadAudio(const char* audio_path,
                                                OgaAudios** audios);

OGA_EXPORT OgaResult* OGA_API_CALL
OgaLoadAudios(const OgaStringArray* audio_paths, OgaAudios** audios);

/**
 * \brief Load multiple audios from an array of byte buffers
 * \param[in] audio_data Array of byte buffers containing the audio data.
 * \param[in] audio_data_sizes Array of sizes of the byte buffers.
 * \param[in] count Number of audios to load.
 * \param[out] audios The loaded audios.
 * \return OgaResult containing the error message if the loading of the audios
 * failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaLoadAudiosFromBuffers(const void** audio_data,
                         const size_t* audio_data_sizes,
                         size_t count,
                         OgaAudios** audios);

OGA_EXPORT void OGA_API_CALL OgaDestroyAudios(OgaAudios* audios);

/**
 * \brief Creates a runtime settings instance to be used to create a model.
 * \param[out] out The created runtime settings.
 * \return OgaResult containing the error message if the creation of the runtime
 * settings failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaCreateRuntimeSettings(OgaRuntimeSettings** out);
/**
 * \brief Destroys the given runtime settings.
 * \param[in] settings The runtime settings to be destroyed.
 */
OGA_EXPORT void OGA_API_CALL
OgaDestroyRuntimeSettings(OgaRuntimeSettings* settings);

/**
 * \brief Sets a specific runtime handle for the runtime settings.
 * \param[in] settings The runtime settings to set the device type.
 * \param[in] handle_name The name of the handle to set for the runtime
 * settings.
 * \param[in] handle The value of handle to set for the runtime settings.
 * \return OgaResult containing the error message if the setting of the device
 * type failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaRuntimeSettingsSetHandle(OgaRuntimeSettings* settings,
                            const char* handle_name,
                            void* handle);

/**
 * \brief Creates an OgaConfig from the given configuration directory.
 * \param[in] config_path The path to the configuration directory. The path is
 * expected to be encoded in UTF-8.
 * \param[out] out The created config.
 * \return OgaResult containing the error message if the creation of the config
 * failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaCreateConfig(const char* config_path,
                                                   OgaConfig** out);

/**
 * \brief Clear the list of providers in the given config
 * \param[in] config The config to clear the providers from.
 * \return OgaResult containing the error message if the clearing of the
 * providers failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaConfigClearProviders(OgaConfig* config);

/**
 * \brief Add the provider at the end of the list of providers in the given
 * config if it doesn't already exist if it already exists, does nothing.
 * \param[in] config The config to set the provider on.
 * \param[in] provider The provider to set on the config.
 * \return OgaResult containing the error message if the setting of the provider
 * failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaConfigAppendProvider(OgaConfig* config, const char* provider);

/**
 * \brief Set a provider option
 * \param[in] config The config to set the provider option on.
 * \param[in] provider The provider to set the option on.
 * \param[in] key The key of the option to set.
 * \param[in] value The value of the option to set.
 * \return OgaResult containing the error message if the setting of the provider
 * option failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaConfigSetProviderOption(OgaConfig* config,
                           const char* provider,
                           const char* key,
                           const char* value);

/**
 * \brief Add the model data to load the model from memory. Applications may
 * call OgaConfigRemoveModelData to remove the model data when it is no longer
 * needed.
 *
 * Note that the model data is expected to be valid at least until the model is
 * created. If using session options such as
 * `session.use_ort_model_bytes_directly`, the model data must remain valid
 * until the OgaModel is destroyed, as the model data will be used directly by
 * the Ort::Session. Please see the relevant ONNX Runtime documentation for more
 * details on this option.
 *
 * \param[in] config The config to add the model data to.
 * \param[in] model_filename The name of the model file as defined in the
 * config.
 * \param[in] model_data The model data to add. The data is expected to be valid
 * at least until the model is created.
 * \param[in] model_data_length The length of the model data.
 * \return OgaResult containing the error message if the addition of the model
 * data failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaConfigAddModelData(OgaConfig* config,
                      const char* model_filename,
                      const void* model_data,
                      size_t model_data_length);

/**
 * \brief Remove model data previously added to the config.
 * \param[in] config The config to remove the model data from.
 * \param[in] model_filename The name of the model file as defined in the
 * config.
 * \return OgaResult containing the error message if the removal of the model
 * data failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaConfigRemoveModelData(OgaConfig* config, const char* model_filename);

/**
 * \brief Overlay JSON on top of config file
 * \param[in] config The config to overlay the JSON on.
 * \param[in] json The JSON to overlay on the config.
 * \return OgaResult containing the error message if the overlaying of the JSON
 * failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaConfigOverlay(OgaConfig* config,
                                                    const char* json);

/**
 * \brief Creates a model from the given configuration directory.
 * \param[in] config_path The path to the model configuration directory. The
 * path is expected to be encoded in UTF-8.
 * \param[out] out The created model.
 * \return OgaResult containing the error message if the model creation failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaCreateModel(const char* config_path,
                                                  OgaModel** out);

/**
 * \brief Creates a model from the given configuration.
 * \param[in] config The configuration to use for the model.
 * \param[out] out The created model.
 * \return OgaResult containing the error message if the model creation failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaCreateModelFromConfig(const OgaConfig* config, OgaModel** out);

/**
 * \brief Creates a model from the given configuration directory, runtime
 * settings and device type.
 * \param[in] config_path The path to the model configuration directory. The
 * path is expected to be encoded in UTF-8.
 * \param[in] settings The runtime settings to use for the model.
 * \param[out] out The created model.
 * \return OgaResult containing the error message if the model creation failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaCreateModelWithRuntimeSettings(const char* config_path,
                                  const OgaRuntimeSettings* settings,
                                  OgaModel** out);

/**
 * \brief Returns the type of the model.
 * \param[in] model The model to get the type from.
 * \param[out] out The type of the model. Must be destroyed with
 * OgaDestroyString
 * \return OgaResult containing the error message if the getting of the model
 * type failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaModelGetType(const OgaModel* model,
                                                   const char** out);

/**
 * \brief Returns the device type of the model.
 * \param[in] model The model to get the device type from.
 * \param[out] out The device type of the model. Must be destroyed with
 * OgaDestroyString
 * \return OgaResult containing the error message if the getting of the device
 * type failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaModelGetDeviceType(const OgaModel* model,
                                                         const char** out);

/**
 * \brief Destroys the given config
 * \param[in] config The config to be destroyed.
 */
OGA_EXPORT void OGA_API_CALL OgaDestroyConfig(OgaConfig* config);

/**
 * \brief Destroys the given model.
 * \param[in] model The model to be destroyed.
 */
OGA_EXPORT void OGA_API_CALL OgaDestroyModel(OgaModel* model);

/**
 * \brief Creates a OgaGeneratorParams from the given model.
 * \param[in] model The model to use for generation.
 * \param[out] out The created generator params.
 * \return OgaResult containing the error message if the generator params
 * creation failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaCreateGeneratorParams(const OgaModel* model, OgaGeneratorParams** out);

/**
 * \brief Destroys the given generator params.
 * \param[in] params The generator params to be destroyed.
 */
OGA_EXPORT void OGA_API_CALL
OgaDestroyGeneratorParams(OgaGeneratorParams* params);

OGA_EXPORT OgaResult* OGA_API_CALL
OgaGeneratorParamsSetSearchNumber(OgaGeneratorParams* params,
                                  const char* name,
                                  double value);
OGA_EXPORT OgaResult* OGA_API_CALL
OgaGeneratorParamsSetSearchBool(OgaGeneratorParams* params,
                                const char* name,
                                bool value);
OGA_EXPORT OgaResult* OGA_API_CALL
OgaGeneratorParamsTryGraphCaptureWithMaxBatchSize(OgaGeneratorParams* params,
                                                  int32_t max_batch_size);

/**
 * \brief Sets the guidance type and data for the Generator params
 * \param[in] params The generator params to set the guidance on
 * \param[in] type The type of the guidance. Currently, we support json_schema,
 * regex and lark_grammar
 * \param[in] data The input string, which is the guidance data. Examples are
 * present in test/test_models/grammars folder
 * \return OgaResult containing the error message if the setting of the guidance
 * failed
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaGeneratorParamsSetGuidance(OgaGeneratorParams* params,
                              const char* type,
                              const char* data);

/**
 * \brief Creates a generator from the given model and generator params.
 * \param[in] model The model to use for generation.
 * \param[in] params The parameters to use for generation.
 * \param[out] out The created generator.
 * \return OgaResult containing the error message if the generator creation
 * failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaCreateGenerator(const OgaModel* model,
                   const OgaGeneratorParams* params,
                   OgaGenerator** out);

/**
 * \brief Destroys the given generator.
 * \param[in] generator The generator to be destroyed.
 */
OGA_EXPORT void OGA_API_CALL OgaDestroyGenerator(OgaGenerator* generator);

/**
 * \brief Returns true if the generator has finished generating all the
 * sequences.
 * \param[in] generator The generator to check if it is done with generating all
 * sequences.
 * \return True if the generator has finished generating all the sequences,
 * false otherwise.
 */
OGA_EXPORT bool OGA_API_CALL OgaGenerator_IsDone(const OgaGenerator* generator);
OGA_EXPORT bool OGA_API_CALL
OgaGenerator_IsSessionTerminated(const OgaGenerator* generator);

/**
 * \brief For additional model inputs that genai does not handle, this lets the
 * user set their values. For example LoRA models handle fine tuning through
 * model inputs. This lets the user supply the fine tuning inputs, while genai
 * handles the standard inputs.
 * \param[in] generator The generator to add the inputs to.
 * \param[in] name Name of the model input (this must match the model's input
 * name)
 * \param[in] tensor The OgaTensor of the input data
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaGenerator_SetModelInput(OgaGenerator* generator,
                           const char* name,
                           OgaTensor* tensor);

/**
 * \brief For additional model inputs that genai does not handle, this lets the
 * user set their values.
 * \param[in] generator The generator to add the inputs to.
 * \param[in] named_tensors The named tensors to set the inputs as.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaGenerator_SetInputs(OgaGenerator* generator,
                       const OgaNamedTensors* named_tensors);

/**
 * \brief Adds the input ids to the generator. The input ids are used to seed
 * the generation.
 * \param[in] generator The generator to add the input ids to.
 * \param[in] p_sequences The input id sequences.
 * \return OgaResult containing the error message if the setting of the input
 * ids failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaGenerator_AppendTokenSequences(OgaGenerator* generator,
                                  const OgaSequences* p_sequences);

/**
 * \brief Adds the input ids to the generator. The input ids are used to seed
 * the generation.
 * \param[in] generator The generator to add the input ids to.
 * \param[in] input_ids The input ids to add.
 * \param[in] input_ids_count The number of input ids to add (batch_size *
 * sequence_length).
 * \return OgaResult containing the error message if the setting of the input
 * ids failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaGenerator_AppendTokens(OgaGenerator* generator,
                          const int32_t* input_ids,
                          size_t input_ids_count);

/**
 * \brief Computes the logits from the model based on the input ids and the past
 * state. The computed logits are stored in the generator.
 * \param[in] generator The generator to compute the logits for.
 * \return OgaResult containing the error message if the computation of the
 * logits failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaGenerator_GenerateNextToken(OgaGenerator* generator);

/**
 * \brief Returns a pointer to the next tokens generated by the model. The
 * out_count will match the batch size
 * \param[in] generator The generator to get the next tokens from.
 * \param[out] out The pointer to the next tokens generated by the model. The
 * pointer is valid until the next OgaGenerator call
 * \param[out] out_count The number of tokens in the out array.
 * \return OgaResult containing the error message if the getting of the next
 * tokens failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaGenerator_GetNextTokens(const OgaGenerator* generator,
                           const int32_t** out,
                           size_t* out_count);

OGA_EXPORT OgaResult* OGA_API_CALL
OgaGenerator_SetRuntimeOption(OgaGenerator* generator,
                              const char* key,
                              const char* value);

/**
 * \brief Rewinds the generator to the given length. This is useful when the
 * user wants to rewind the generator to a specific length and continue
 * generating from that point.
 * \param[in] generator The generator to rewind to the given length.
 * \param[in] new_length The desired length in tokens after rewinding.
 * \return OgaResult containing the error message if the rewinding failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaGenerator_RewindTo(OgaGenerator* generator, size_t new_length);

/**
 * \brief Returns a copy of the model input identified by the given name as an
 * OgaTensor on CPU. The buffer is owned by returned OgaTensor and will be
 * released when the OgaTensor is destroyed
 * \param[in] generator The generator to run the GetInput on the name provided
 * and the out pointer to store the input.
 * \param[in] name The name of the input tensor.
 * \param[out] out The returned OgaTensor.
 * \return OgaResult containing the error message if the computation failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaGenerator_GetInput(const OgaGenerator* generator,
                      const char* name,
                      OgaTensor** out);

/**
 * \brief Returns a copy of the model output identified by the given name as an
 * OgaTensor on CPU. The buffer is owned by returned OgaTensor and will be
 * released when the OgaTensor is destroyed
 * \param[in] generator The generator to run the GetOutput on the name provided
 * and the out pointer to store the output.
 * \param[in] name The name of the output tensor.
 * \param[out] out The returned OgaTensor.
 * \return OgaResult containing the error message if the computation failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaGenerator_GetOutput(const OgaGenerator* generator,
                       const char* name,
                       OgaTensor** out);

/**
 * \brief Returns a copy of the logits from the model as an OgaTensor on CPU.
 * The buffer is owned by returned OgaTensor and will be released when the
 * OgaTensor is destroyed
 * \param[in] generator The generator get the logits from
 * \param[out] out The OgaTensor containing the logits, it only contains the
 * last token logits even in prompt processing
 * \return OgaResult containing the error message if the computation failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaGenerator_GetLogits(OgaGenerator* generator, OgaTensor** out);

/**
 * \brief Sets the logits to the generator. This is useful when the user wants
 * to set the logits to a specific value for example when doing guided
 * generation.
 * \param[in] generator The generator to set the logits on
 * \param[in] tensor The OgaTensor containing the logits, it must have the same
 * shape as the logits returned by GetLogits which is the last token logits.
 * \return OgaResult containing the error message if the setting of the logits
 * failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaGenerator_SetLogits(OgaGenerator* generator, OgaTensor* tensor);

/*
 * \brief Returns the number of tokens in the sequence at the given index.
 * \param[in] generator The generator to get the count of the tokens for the
 * sequence at the given index.
 * \param[in] index The given index.
 * \return The number tokens in the sequence at the given index.
 */
OGA_EXPORT size_t OGA_API_CALL
OgaGenerator_GetSequenceCount(const OgaGenerator* generator, size_t index);

/**
 * \brief Returns a pointer to the sequence data at the given index. The number
 * of tokens in the sequence is given by OgaGenerator_GetSequenceCount
 * \param[in] generator The generator to get the sequence data for the sequence
 * at the given index.
 * \param[in] index The given index.
 * \return The pointer to the sequence data at the given index. The sequence
 * data is owned by the OgaGenerator and will be freed when the OgaGenerator is
 * destroyed. The caller must copy the data if it needs to be used after the
 * OgaGenerator is destroyed.
 */
OGA_EXPORT const int32_t* OGA_API_CALL
OgaGenerator_GetSequenceData(const OgaGenerator* generator, size_t index);

OGA_EXPORT OgaResult* OGA_API_CALL OgaCreateTokenizer(const OgaModel* model,
                                                      OgaTokenizer** out);
OGA_EXPORT void OGA_API_CALL OgaDestroyTokenizer(OgaTokenizer*);

OGA_EXPORT OgaResult* OGA_API_CALL
OgaCreateMultiModalProcessor(const OgaModel* model,
                             OgaMultiModalProcessor** out);

OGA_EXPORT void OGA_API_CALL
OgaDestroyMultiModalProcessor(OgaMultiModalProcessor* processor);

/**
 * Encodes a single string and adds the encoded sequence of tokens to the
 * OgaSequences. The OgaSequences must be freed with OgaDestroySequences when it
 * is no longer needed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaTokenizerEncode(const OgaTokenizer*,
                                                      const char* str,
                                                      OgaSequences* sequences);

/**
 * Batch encode an array of strings and return a single tensor output
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaTokenizerEncodeBatch(const OgaTokenizer*,
                                                           const char** strings,
                                                           size_t count,
                                                           OgaTensor** out);

/**
 * Batch decode a tensor of token ids and return an array of strings
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaTokenizerDecodeBatch(const OgaTokenizer*,
                        const OgaTensor* tensor,
                        OgaStringArray** out);

/**
 * \brief Converts the given string to a single token id.
 * \param[in] tokenizer The tokenizer to use to convert the string to a token
 * id.
 * \param[in] str The string to convert to a token id.
 * \param[in] token_id The converted token id.
 * \return OgaResult containing the error message if the conversion of the
 * string to a token id failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaTokenizerToTokenId(const OgaTokenizer* tokenizer,
                      const char* str,
                      int32_t* token_id);

/**
 * \brief Process images with input prompt
 * \param[in] processor The processor to use to process the images and prompt.
 * \param[in] prompt The prompt to use with the images.
 * \param[in] images The images to process.
 * \return OgaResult containing the named tensors for the processed inputs.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaProcessorProcessImages(const OgaMultiModalProcessor*,
                          const char* prompt,
                          const OgaImages* images,
                          OgaNamedTensors** input_tensors);

/**
 * \brief Process images with input prompts
 * \param[in] processor The processor to use to process the images and prompts.
 * \param[in] prompts The prompts to use with the images.
 * \param[in] images The images to process.
 * \return OgaResult containing the named tensors for the processed inputs.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaProcessorProcessImagesAndPrompts(const OgaMultiModalProcessor*,
                                    const OgaStringArray* prompts,
                                    const OgaImages* images,
                                    OgaNamedTensors** input_tensors);

/**
 * \brief Process audios with input prompt
 * \param[in] processor The processor to use to process the audios and prompt.
 * \param[in] prompt The prompt to use with the audios.
 * \param[in] audios The audios to process.
 * \return OgaResult containing the named tensors for the processed inputs.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaProcessorProcessAudios(const OgaMultiModalProcessor*,
                          const char* prompt,
                          const OgaAudios* audios,
                          OgaNamedTensors** input_tensors);

/**
 * \brief Process audios with input prompts
 * \param[in] processor The processor to use to process the audios and prompts.
 * \param[in] prompts The prompts to use with the audios.
 * \param[in] audios The audios to process.
 * \return OgaResult containing the named tensors for the processed inputs.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaProcessorProcessAudiosAndPrompts(const OgaMultiModalProcessor*,
                                    const OgaStringArray* prompts,
                                    const OgaAudios* audios,
                                    OgaNamedTensors** input_tensors);

/**
 * \brief Process images and/or audios with input prompt
 * \param[in] processor The processor to use to process the images, audios,
 * and/or prompt.
 * \param[in] prompt The prompt to use with the images and/or audios.
 * \param[in] images The images to process.
 * \param[in] audios The audios to process.
 * \return OgaResult containing the named tensors for the processed inputs.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaProcessorProcessImagesAndAudios(const OgaMultiModalProcessor*,
                                   const char* prompt,
                                   const OgaImages* images,
                                   const OgaAudios* audios,
                                   OgaNamedTensors** input_tensors);

/**
 * \brief Process images and/or audios with input prompts
 * \param[in] processor The processor to use to process the images, audios,
 * and/or prompts.
 * \param[in] prompts The prompts to use with the images and/or audios.
 * \param[in] images The images to process.
 * \param[in] audios The audios to process.
 * \return OgaResult containing the named tensors for the processed inputs.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaProcessorProcessImagesAndAudiosAndPrompts(const OgaMultiModalProcessor*,
                                             const OgaStringArray* prompts,
                                             const OgaImages* images,
                                             const OgaAudios* audios,
                                             OgaNamedTensors** input_tensors);

/** Decode a single token sequence and returns a null terminated utf8 string.
 * out_string must be freed with OgaDestroyString
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaTokenizerDecode(const OgaTokenizer*,
                                                      const int32_t* tokens,
                                                      size_t token_count,
                                                      const char** out_string);
OGA_EXPORT OgaResult* OGA_API_CALL
OgaProcessorDecode(const OgaMultiModalProcessor*,
                   const int32_t* tokens,
                   size_t token_count,
                   const char** out_string);

/**
 * @brief Applies a chat template to input messages
 *
 * This function processes the specified template with the provided input using
 * the tokenizer, and outputs the resulting string. Optionally, it can include a
 * generation prompt in the output.
 *
 * \param[in] tokenizer OgaTokenizer used for template processing.
 * \param[in] template_str Null-terminated string representing the chat
 * template. Use nullptr to fall back to the default chat template from the
 * tokenizer config.
 * \param[in] messages Null-terminated string containing the input messages to
 * be processed.
 * \param[in] tools Null-terminated string containing the chat function calls if
 * any. Use nullptr if none.
 * \param[in] add_generation_prompt Indicates whether to add a generation prompt
 * to the output.
 * \param[out] out_string Pointer to where the output will be stored. The
 * returned pointer must be freed with OgaDestroyString
 * \return OgaResult* containing the error message if the function fails
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaTokenizerApplyChatTemplate(const OgaTokenizer*,
                              const char* template_str,
                              const char* messages,
                              const char* tools,
                              bool add_generation_prompt,
                              const char** out_string);

/** OgaTokenizerStream is to decoded token strings incrementally, one token at a
 * time.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaCreateTokenizerStream(const OgaTokenizer*, OgaTokenizerStream** out);
OGA_EXPORT OgaResult* OGA_API_CALL
OgaCreateTokenizerStreamFromProcessor(const OgaMultiModalProcessor*,
                                      OgaTokenizerStream** out);
OGA_EXPORT void OGA_API_CALL OgaDestroyTokenizerStream(OgaTokenizerStream*);

/**
 * Decode a single token in the stream. If this results in a word being
 * generated, it will be returned in 'out'. The caller is responsible for
 * concatenating each chunk together to generate the complete result. 'out' is
 * valid until the next call to OgaTokenizerStreamDecode or when the
 * OgaTokenizerStream is destroyed
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaTokenizerStreamDecode(OgaTokenizerStream*,
                                                            int32_t token,
                                                            const char** out);

/** Create an OgaTensor from an optional user owned buffer. If a user owned
 * buffer is supplied, the OgaTensor does not own the memory (as it has no way
 * to free it) so the 'data' parameter must be valid for the lifetime of the
 * OgaTensor. If the 'data' parameter is nullptr, the OgaTensor will allocate
 * its own memory.
 *
 * \param[in] data User supplied memory pointer, if non nullptr it must remain
 * valid for lifetime of the OgaTensor
 * \param[in] shape_dims Pointer to array of int64_t values that define the
 * tensor shape, example [1 20 30] would be equivalent to a C array of
 * [1][20][30]
 * \param[in] shape_dims_count Count of elements in the shape_dims array
 * \param[in] element_type The data type that 'data' points to.
 * \param[out] out Writes the newly created OgaTensor into this, must be
 * destroyed with OgaDestroyTensor
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaCreateTensorFromBuffer(void* data,
                          const int64_t* shape_dims,
                          size_t shape_dims_count,
                          OgaElementType element_type,
                          OgaTensor** out);

OGA_EXPORT void OGA_API_CALL OgaDestroyTensor(OgaTensor* tensor);

/** Get the OgaElementType of the data stored in the OgaTensor
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaTensorGetType(OgaTensor*,
                                                    OgaElementType* out);

/** Get the number of dimensions of the OgaTensor's shape, typically used to
 * allocate a buffer of this size then calling OgaTensorGetShape with it
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaTensorGetShapeRank(OgaTensor*,
                                                         size_t* out);

/** Copies the shape dimensions into the shape_dims parameters. shape_dims_count
 * must match the value returned by OgaTensorGetShapeRank
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaTensorGetShape(OgaTensor*,
                                                     int64_t* shape_dims,
                                                     size_t shape_dims_count);

/** A pointer to the tensor data, it is typically cast into the actual data type
 * of the tensor
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaTensorGetData(OgaTensor*, void** out);

/** \brief Create an OgaNamedTensors
 * \param[out] out The created OgaNamedTensors
 * \return OgaResult containing the error message if the creation of the
 * OgaNamedTensors failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaCreateNamedTensors(OgaNamedTensors** out);

/** \brief Lookup a tensor in a NamedTensor set by name
 * \param[in] named_tensors The named tensors to search
 * \param[in] name The name of the tensor to find
 * \param[out] out The tensor with the given name
 * \return OgaResult containing the error message if the tensor with the given
 * name could not be found.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaNamedTensorsGet(OgaNamedTensors* named_tensors,
                   const char* name,
                   OgaTensor** out);

/** \brief Set a tensor in a NamedTensor set by name
 * \param[in] named_tensors The named tensors to set the tensor
 * \param[in] name The name of the tensor to set
 * \param[in] tensor The tensor to set
 * \return OgaResult containing the error message if the tensor with the given
 * name could not be set.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaNamedTensorsSet(OgaNamedTensors* named_tensors,
                   const char* name,
                   OgaTensor* tensor);

/** \brief Delete a tensor in a NamedTensor set by name
 * \param[in] named_tensors The named tensors to remove the tensor
 * \param[in] name The name of the tensor to remove
 * \return OgaResult containing the error message if the tensor with the given
 * name could not be removed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaNamedTensorsDelete(OgaNamedTensors* named_tensors, const char* name);

/** \brief Get the number of tensors in the NamedTensors
 * \param[in] named_tensors The named tensors to get the count of the tensors
 * \param[out] out The number of tensors in the NamedTensors
 * \return OgaResult containing the error message if the getting of the count of
 * the tensors failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaNamedTensorsCount(const OgaNamedTensors* named_tensors, size_t* out);

/** \brief Return an OgaStringArray of the names of the tensors in an
 * OgaNamedTensors object
 * \param[in] named_tensors The named tensors to get the names of the tensors
 * \param[out] out The OgaStringArray containing the names of the tensors
 * \return OgaResult containing the error message if the getting of the names of
 * the tensors failed.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaNamedTensorsGetNames(const OgaNamedTensors* named_tensors,
                        OgaStringArray** out);

OGA_EXPORT OgaResult* OGA_API_CALL OgaSetCurrentGpuDeviceId(int device_id);
OGA_EXPORT OgaResult* OGA_API_CALL OgaGetCurrentGpuDeviceId(int* device_id);

/**
 * \brief Creates an object of type OgaStringArray.
 * \return The result of the operation. If the operation is successful, a
 * nullptr is returned.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaCreateStringArray(OgaStringArray** out);

/**
 * \brief Creates an object of type OgaStringArray from the given strings.
 * \return The result of the operation. If the operation is successful, a
 * nullptr is returned.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaCreateStringArrayFromStrings(const char* const* strs,
                                size_t count,
                                OgaStringArray** out);

/**
 * \brief Destroys OgaStringArray.
 */
OGA_EXPORT void OGA_API_CALL
OgaDestroyStringArray(OgaStringArray* string_array);

/**
 * \brief Adds the given string to the string_array.
 * \param[inout] string_array The string array to which the string is to be
 * added
 * \param[in] str The string to be added to the string_array.
 * \return The result of the operation. If the operation is successful, a
 * nullptr is returned.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaStringArrayAddString(OgaStringArray* string_array, const char* str);

/**
 * \brief Gets the number of strings in the string_array.
 * \param[in] string_array The OgaStringArray object to get the count of the
 * strings.
 * \param[out] out The number of strings in the string_array.
 * \return The result of the operation. If the operation is successful, a
 * nullptr is returned.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaStringArrayGetCount(const OgaStringArray* string_array, size_t* out);

/**
 * \brief Get a string from a string_array
 * \param[in] string_array The OgaStringArray object to get the string from.
 * \param[in] index The index of the string to get.
 * \return The string at the given index.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaStringArrayGetString(const OgaStringArray* string_array,
                        size_t index,
                        const char** out);

/**
 * \brief Creates the OgaAdapters object that manages the adapters.
          - The OgaAdapters object is used to load all the model adapters.
          - It is responsible for reference counting the loaded adapters.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaCreateAdapters(const OgaModel* model,
                                                     OgaAdapters** out);

/**
 * \brief Destroys the OgaAdapters object.
 */
OGA_EXPORT void OGA_API_CALL OgaDestroyAdapters(OgaAdapters* adapters);

/**
 * \brief Loads the model adapter from the given adapter file path and adapter
 * name.
 * \param[in] adapters The OgaAdapters object to load the adapter.
 * \param[in] adapter_file_path The file path of the adapter to load.
 * \param[in] adapter_name A unique identifier for the adapter chosed by the
 * function invoker. This name is used for querying the adapter.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaLoadAdapter(OgaAdapters* adapters,
                                                  const char* adapter_file_path,
                                                  const char* adapter_name);

/**
 * \brief Unloads the adapter with the given identifier from the previosly
 loaded adapters. If the adapter is not found, or if it cannot be unloaded (when
 it is in use), an error is returned.
 * \param[in] adapters The OgaAdapters object to unload the adapter.
 * \param[in] adapter_name The name of the adapter to unload.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaUnloadAdapter(OgaAdapters* adapters,
                                                    const char* adapter_name);

/**
 * \brief Sets the adapter with the given adapter name as active for the given
 * OgaGenerator object.
 * \param[in] generator The OgaGenerator object to set the active adapter.
 * \param[in] adapters The OgaAdapters object that manages the model adapters.
 * \param[in] adapter_name The name of the adapter to set as active.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaSetActiveAdapter(OgaGenerator* generator,
                    OgaAdapters* adapters,
                    const char* adapter_name);

/**
 * \brief Creates an OgaEngine object from the given model.
 *
 * The OgaEngine is responsible for managing and scheduling multiple requests,
 * executing model inference, and coordinating batching, caching, and resource
 * management for efficient processing. This function initializes a new engine
 * instance using the provided model, allowing requests to be added, removed,
 * and processed through the engine's API. The engine must be destroyed with
 * OgaDestroyEngine when no longer needed.
 *
 * \param[in] model The model to use for the engine. The model must remain valid
 * for the lifetime of the engine.
 * \param[out] out Pointer to the created engine instance. On success, *out will
 * be set to the new engine object.
 * \return OgaResult containing the error message if the engine creation failed,
 * or nullptr on success.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaCreateEngine(OgaModel* model,
                                                   OgaEngine** out);

/**
 * \brief Destroys the given engine.
 * \param[in] engine The engine to be destroyed.
 */
OGA_EXPORT void OGA_API_CALL OgaDestroyEngine(OgaEngine* engine);

/**
 * \brief Returns a ready request of runs one step of the OgaEngine if there are
 * pending requests.
 *
 * This function advances the state of the engine by processing a subset of the
 * currently pending requests. It schedules and executes model inference for
 * requests that are ready, updates their state with the generated results, and
 * manages batching and resource allocation as needed. This function should be
 * called repeatedly (e.g., in a loop) to ensure all requests are processed
 * efficiently. It is a core part of the engine's request processing pipeline.
 * If the engine has ready requests from a previous call, it will return one of
 * them in the request parameter. If there are no ready requests, a new subset
 * of requests will be scheduled for processing and the request parameter will
 * be set to the first request from this subset that is ready to be queried for
 * results.
 *
 * \param[in] engine The engine instance to run a processing step on.
 * \param[out] request A request that has been processed by the engine and is
 * ready to be queried for results. If the engine has no ready requests, this
 * will be set to a nullptr.
 * \return OgaResult containing the error message if the operation failed, or
 * nullptr on success.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaEngineStep(OgaEngine* engine,
                                                 OgaRequest** request);

/**
 * \brief Checks if the engine has any pending requests to process.
 *
 * This function queries the OgaEngine to determine whether there are any
 * requests that have not yet been fully processed.
 *
 * \param[in] engine The engine instance to check for pending requests.
 * \param[out] out Pointer to a boolean value that will be set to true if there
 * are pending requests, or false otherwise.
 * \return OgaResult containing the error message if the operation failed, or
 * nullptr on success.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaEngineHasPendingRequests(OgaEngine* engine, bool* out);

/**
 * \brief Adds a request to the OgaEngine for processing.
 *
 * This function submits a new request to the engine, which will then be
 * processed in subsequent calls to OgaEngineStep. The request must be created
 * using OgaCreateRequest and should contain the necessary parameters for model
 * inference.
 *
 * \param[in] engine The engine instance to which the request is being added.
 * \param[in] request The request to add to the engine. The request must remain
 * valid until it is removed or processed.
 * \return OgaResult containing the error message if the operation failed, or
 * nullptr on success.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaEngineAddRequest(OgaEngine* engine,
                                                       OgaRequest* request);

/**
 * \brief Removes a request from the OgaEngine.
 *
 * This function removes a request from the engine, allowing it to be cleaned
 * up. The request must have been previously added to the engine using
 * OgaEngineAddRequest. After this call, the request will no longer be processed
 * by the engine.
 *
 * \param[in] engine The engine instance from which the request is being
 * removed.
 * \param[in] request The request to remove from the engine. The request must
 * have been previously added to the engine.
 * \return OgaResult containing the error message if the operation failed, or
 * nullptr on success.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaEngineRemoveRequest(OgaEngine* engine,
                                                          OgaRequest* request);

/**
 * \brief Creates a new request for the OgaEngine.
 *
 * This function initializes a new request object that can be used to submit
 * input sequences for model inference. Once added to the engine, the request
 * will be processed by the engine in subsequent calls to OgaEngineStep.
 *
 * \param[in] params The parameters for the generator, such as temperature,
 * top-k, etc.
 * \param[out] out Pointer to the created request instance. On success, *out
 * will be set to the new request object.
 * \return OgaResult containing the error message if the request creation
 * failed, or nullptr on success.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaCreateRequest(OgaGeneratorParams* params,
                                                    OgaRequest** out);

/**
 * \brief Adds input sequences to the request.
 *
 * This function sets the input sequences for the request. The input sequences
 * are used to seed the generation process. The request must have been created
 * using OgaCreateRequest before calling this function.
 *
 * \param[in] request The request to set the input sequences on.
 * \param[in] tokens The input sequences to set on the request.
 * \return OgaResult containing the error message if the setting of the input
 * sequences failed, or nullptr on success.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaRequestAddTokens(OgaRequest* request, const OgaSequences* tokens);

/**
 * \brief Destroys the given request.
 *
 * This function cleans up the resources associated with the request, including
 * any input sequences and parameters. It should be called when the request is
 * no longer needed, either after it has been processed.
 *
 * \param[in] request The request to be destroyed. The request must have been
 * created using OgaCreateRequest.
 */
OGA_EXPORT void OGA_API_CALL OgaDestroyRequest(OgaRequest* request);

/**
 * \brief Sets custom user data on the request.
 *
 * This function sets custom user data on the request that is opaque to the
 * request. It can be queried later using OgaRequestGetOpaqueData. This is
 * useful for associating additional information with the request that may be
 * actionable by the user or application logic.
 *
 * \param[in] request The request to set the input sequences on.
 * \param[in] tokens The input sequences to set on the request.
 * \return OgaResult containing the error message if the setting of the input
 * sequences failed, or nullptr on success.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaRequestSetOpaqueData(OgaRequest* request,
                                                           void* opaque_data);

/**
 * \brief Gets the custom user data from the request.
 *
 * This function retrieves the custom user data that was set on the request
 * using OgaRequestSetOpaqueData. The user data is opaque to the request and can
 * be used to store additional information that may be useful for the
 * application logic.
 *
 * \param[in] request The request to get the opaque data from.
 * \param[out] opaque_data Pointer to where the opaque data will be stored.
 * \return OgaResult containing the error message if the getting of the opaque
 * data failed, or nullptr on success.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaRequestGetOpaqueData(OgaRequest* request,
                                                           void** opaque_data);

/**
 * \brief Checks if the request has any unseen tokens.
 *
 * This function checks if the request has any unseen tokens that have not yet
 * been queried by the user or application yet. Unseen tokens are those that
 * have been generated by the model but not yet retrieved by the user.
 *
 * \param[in] request The request to check for unseen tokens.
 * \param[out] out Boolean flag that will be set to true if there are unseen
 * tokens, or false otherwise.
 * \return OgaResult containing the error message if the setting of the input
 * sequences failed, or nullptr on success.
 */
OGA_EXPORT OgaResult* OGA_API_CALL
OgaRequestHasUnseenTokens(const OgaRequest* request, bool* out);

/**
 * \brief Gets an unseen token from the request.
 *
 * This function retrieves the next unseen token from the request. If there are
 * no unseen tokens, it will return an error. The unseen token is a token that
 * has been generated by the model but has not yet been queried by the user.
 *
 * \param[in] request The request to get the unseen token from.
 * \param[out] out Pointer to where the unseen token will be stored.
 * \return OgaResult containing the error message if the getting of the unseen
 * token failed, or nullptr on success.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaRequestGetUnseenToken(OgaRequest* request,
                                                            int32_t* out);

/**
 * \brief Checks if the request is done processing.
 *
 * This function checks if the request has finished processing. The request is
 * done when one of the termination conditions has been reached (e.g. end of
 * sequence token is encountered or the request was cancelled). If the request
 * is done, it will return true; otherwise, it will return false.
 *
 * \param[in] request The request to check if it is done.
 * \param[out] out Boolean flag that will be set to true if the request is done,
 * or false otherwise.
 * \return OgaResult containing the error message if the checking of the request
 * status failed, or nullptr on success.
 */
OGA_EXPORT OgaResult* OGA_API_CALL OgaRequestIsDone(const OgaRequest* request,
                                                    bool* out);

/**
 * \brief Registers an execution provider library with ONNXRuntime API.
 * \param registration_name name for registration.
 * \param path provider path.
 *
 */
OGA_EXPORT void OGA_API_CALL
OgaRegisterExecutionProviderLibrary(const char* registration_name,
                                    const char* library_path);

/**
 * \brief Unregisters an execution provider library with ONNXRuntime API.
 * \param registration_name name for registration.
 *
 */
OGA_EXPORT void OGA_API_CALL
OgaUnregisterExecutionProviderLibrary(const char* registration_name);

#ifdef __cplusplus
}
#endif
//! @}
