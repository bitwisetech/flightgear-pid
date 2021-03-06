if (SYSTEM_FLITE)
    set(Flite_FIND_REQUIRED TRUE)
    find_package(Flite)

    include_directories(${FLITE_INCLUDE_DIR})
    message(STATUS "Using Flite includes at: ${FLITE_INCLUDE_DIR}")
    message(STATUS "Using Flite libraries: ${FLITE_LIBRARIES}")
else()
    include_directories(${PROJECT_SOURCE_DIR}/3rdparty/flite_hts_engine/flite/include)
    include_directories(${PROJECT_SOURCE_DIR}/3rdparty/flite_hts_engine/flite/lang/cmulex)
    include_directories(${PROJECT_SOURCE_DIR}/3rdparty/flite_hts_engine/flite/lang/usenglish)

    set(FLITE_SOURCES
        flite/src/hrg/cst_ffeature.c
        flite/src/hrg/cst_item.c
        flite/src/hrg/cst_relation.c
        flite/src/hrg/cst_utterance.c

        flite/src/lexicon/cst_lexicon.c
        flite/src/lexicon/cst_lts.c

        flite/src/regex/cst_regex.c
        flite/src/regex/regexp.c

        flite/src/stats/cst_cart.c

        flite/src/synth/cst_ffeatures.c
        flite/src/synth/cst_phoneset.c
        flite/src/synth/cst_synth.c
        flite/src/synth/cst_utt_utils.c
        flite/src/synth/cst_voice.c
        flite/src/synth/flite.c

        flite/src/utils/cst_alloc.c
        flite/src/utils/cst_error.c
        flite/src/utils/cst_features.c
        flite/src/utils/cst_string.c
        flite/src/utils/cst_tokenstream.c
        flite/src/utils/cst_val.c
        flite/src/utils/cst_val_const.c
        flite/src/utils/cst_val_user.c
    )

    set(FLITE_LANG_SOURCES
        flite/lang/cmu_us_kal/cmu_us_kal.c

        flite/lang/cmulex/cmu_lex.c
        flite/lang/cmulex/cmu_lex_data.c
        flite/lang/cmulex/cmu_lex_entries.c
        flite/lang/cmulex/cmu_lts_model.c
        flite/lang/cmulex/cmu_lts_rules.c
        flite/lang/cmulex/cmu_postlex.c

        flite/lang/usenglish/us_aswd.c
        flite/lang/usenglish/us_expand.c
        flite/lang/usenglish/us_ffeatures.c
        flite/lang/usenglish/us_gpos.c
        flite/lang/usenglish/us_int_accent_cart.c
        flite/lang/usenglish/us_int_tone_cart.c
        flite/lang/usenglish/us_nums_cart.c
        flite/lang/usenglish/us_phoneset.c
        flite/lang/usenglish/us_phrasing_cart.c
        flite/lang/usenglish/us_pos_cart.c
        flite/lang/usenglish/us_text.c
        flite/lang/usenglish/usenglish.c
    )

    if (MSVC)
        add_definitions( -DNO_UNION_INITIALIZATION )
    endif ()
endif()

if (SYSTEM_HTS_ENGINE)
    set(HTS_Engine_FIND_REQUIRED TRUE)
    find_package(HtsEngine)

    include_directories(${HTS_ENGINE_INCLUDE_DIR})
    message(STATUS "Using HTS engine includes for Flite/HTS Engine at: ${HTS_ENGINE_INCLUDE_DIR}")
else()
    include_directories(${PROJECT_SOURCE_DIR}/3rdparty/hts_engine_API/include)
endif()


include_directories(${PROJECT_SOURCE_DIR}/3rdparty/flite_hts_engine/include)

add_definitions(-DFLITE_PLUS_HTS_ENGINE)

add_library(flite_hts STATIC 
    ${FLITE_SOURCES}
    ${FLITE_LANG_SOURCES}
    lib/flite_hts_engine.c
)

target_compile_definitions(flite_hts PRIVATE -DSIZEOF_VOID_P=${CMAKE_SIZEOF_VOID_P})

if (SYSTEM_FLITE)
    target_link_libraries(flite_hts ${FLITE_LIBRARIES})
endif()

