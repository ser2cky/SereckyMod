#pragma once

extern void R_RunParticleEffect(vec3_t org, vec3_t dir, int color, int count);
extern void R_RocketTrail(vec3_t start, vec3_t end, int type);

extern void R_BlobExplosion(vec3_t org);
extern void R_ParticleExplosion(vec3_t org);
extern void R_LavaSplash(vec3_t org);
extern void R_TeleportSplash(vec3_t org);

extern void R_RailTrail(vec3_t start, vec3_t end, vec3_t angles);