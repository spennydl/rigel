# Rigel Rodmap

## TODO's:

 - level loading needs work. i think it's time to rip out tinyxml and say thanks for your service.
   - load lights and zones from json
   - create the concept of trigger zones, load from json (sorta done)
   - hot-reloading
   - use trigger zones to switch between levels
 - text rendering!
 - lighting
   - BIG tech debt in the shader architecture. Hoooooowee. Needs a good afternoon's worth of cleanup.
   - Tuning! Need to be able to set and tune:
     - ambient lighting
     - background gradient
     - light attenuation coefficients
     - HDR exposure parameter (if we even HDR at all)
   - create a circle light type (maybe? I dunno if I actually want this)
   - we need a notion of entity attachments so we can attach lights to entities
 - pull player behavior out into a more final place. Was thinking a brain sorta abstraction,
   but I dunno. (sorta done, needs more time in the oven)
 - controller input

## DONE

- store lights in the world chunk and render them from there
- start laying in lighting.
- we need a proper math lib. if we could get rid of glm i'd be super happy too. This may
  be a good task for a slow day.
- there is work to do on collision, but collision is a bloody pit.
- Switch to loading from json and rip out tinyxml2
- load entities from json exported from tiles
- debug line rendering
- Get it out of main. Time to construct a proper game update step
  and a separate physics update step.
- Load the player sprite
- Figure out how to store their aabb? player metadata?
- Develop the parsing for tmx level data - need a good attribute scheme
- load the tmx level data for the test level
- load the test level image and render it
- Finish moving to SAT collision and support slopes
  - this is working! well enough for a start, anyhow
- Migrate to CMake and vendor dependencies
  - done!
- Finish test level artwork
  - mostly done. Still more to do, but there is enough.
