# Rigel Rodmap

## TODO's:

- Resource management
  - Develop the parsing for tmx level data - need a good attribute scheme
  - load the tmx level data for the test level
  - load the test level image and render it
- Player
  - Load the player sprite
  - Figure out how to store their aabb? player metadata?
- Physics
  - Get it out of main. Time to construct a proper game update step
    and a separate physics update step.
    During game update, entities should each set their velocity to their
    desired velocity. We then run the simulation and entities _may_ move
    by that velocity if they don't collide anything.
    We do need their _actual_ velocity somewhere though, don't we? If
    an entity hits a wall, should their velocity stay the same?
    I think that only matters for velocities that we integrate, like
    gravity. I think I may want to move towards integrating xspeed as
    well, I think that a short ramp-up/down time will fit the game pretty
    well. What we probably want, then, is for entities to set their desired
    acceleration each update.
- CSR for storing level colliders

## DONE

- Finish moving to SAT collision and support slopes
  - this is working! well enough for a start, anyhow
- Migrate to CMake and vendor dependencies
  - done!
- Finish test level artwork
  - mostly done. Still more to do, but there is enough.
