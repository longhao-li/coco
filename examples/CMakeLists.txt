set(
    COCO_EXAMPLE_PROJECTS
    echo
    timer
    copy
    yield
    mutex
)

foreach(COCO_PROJECT ${COCO_EXAMPLE_PROJECTS})
    file(GLOB_RECURSE COCO_EXAMPLE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${COCO_PROJECT}/*.cpp)
    add_executable(${COCO_PROJECT} ${COCO_EXAMPLE_SOURCE})
    target_link_libraries(${COCO_PROJECT} PRIVATE coco::coco)
endforeach(COCO_PROJECT ${COCO_EXAMPLE_PROJECTS})

