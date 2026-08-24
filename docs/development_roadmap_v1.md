# WorldSimDemo Development Roadmap V1

> Status: summary document. The authoritative hierarchical execution plan is
> `master_implementation_plan_v1.md`. If the two documents differ, follow the
> master implementation plan.

## 1. Purpose

This document is the active implementation roadmap for the UE 5.8 project.
Older `day*`, `stage*`, and `phase*` documents are design references only. Their
sample APIs are not authoritative because some use an older architecture based
on `UGameInstanceSubsystem`, `FDateTime`, and integer region identifiers.

The current implementation baseline is:

- `UWorldSubsystem` for world-scoped simulation services
- `FWorldTime` with integer simulation minutes
- `FName` region identifiers
- `TruthLedgerSubsystem` as the source of historical truth
- `CommitmentSubsystem` for future obligations
- `KnowledgeSubsystem` for non-omniscient information
- Unreal Engine 5.8

## 2. Current implementation status

### Implemented in source, not yet compiled in UE 5.8

- UE C++ project and runtime module
- World clock and manual time advancement
- Region snapshots
- Lightweight and instantiated person records
- Daily schedule skeleton
- Presence intervals
- Truth event ledger
- Dialogue-created commitments
- Commitment conflict checks
- Automatic commitment progression
- Deterministic commitment outcomes: success, failure, injury, missing, death
- Persistent life state and current region in causal person state
- Basic causal motivation scoring
- Time-based hunger, fatigue, health, stress, money, and social needs
- Data-driven opportunities with region, goal, duration, capacity, reward, danger
- Active activity lifecycle and outcome application
- Public reports separated from historical truth
- Per-person sparse message knowledge

### Partially implemented

- Commitment and presence consistency: commitments move causal location, but
  presence intervals are not yet generated and reconciled automatically
- Daily life: schedules exist, but they do not yet generate opportunities and
  commitments for a complete day
- Information propagation: public reports and personal knowledge exist, but
  relationships, communication delay, rumours, and corrections do not
- Local NPC detail: lightweight/full records exist, but no Actor spawning,
  animation, interaction, or detail-level transitions have been validated
- Region simulation: snapshots exist, but there is no aggregate economy,
  population, organization, or resource update loop

### Not implemented

- UE 5.8 compilation and Blueprint smoke test
- Save/load and schema versioning
- Player identity creation, character customization, and causal birth location
- A seeded city/spaceport test world
- Hot/warm/cold data lifecycle and archival
- Simulation LOD for local people versus distant populations
- Relationships, households, organizations, jobs, markets, and ownership
- Witnesses, evidence, reporting, police dispatch, cases, and investigations
- Planetary transport and interstellar travel services
- News delivery delay across regions and star systems
- Cultivation attributes, system UI, skills, combat, enemies, and progression
- Alien fleet timeline and civilization-level autonomous decisions
- Planet generation, seamless space traversal, rendering, and art pipeline

## 3. Ordered implementation plan

### Milestone 0: UE 5.8 compile gate

Goal: establish a trustworthy build baseline before adding more systems.

- Generate Visual Studio project files on Windows
- Compile Development Editor with Unreal Header Tool
- Fix reflection, include-order, API, and target-rule errors
- Open a blank map and access every subsystem from Blueprint
- Create one person, one causal state, and one opportunity
- Advance time and inspect the resulting activity and truth events

Exit condition: the project opens in UE 5.8 and the minimal simulation loop runs
without C++ or Blueprint errors.

### Milestone 1: consistency kernel

Goal: make one person's location, commitments, activities, life state, truth,
and knowledge impossible to contradict.

- Connect commitment travel states to presence intervals
- Cancel or interrupt activities when a higher-priority hard event occurs
- Add query APIs for all active commitments, activities, events, and messages
- Add deterministic event ordering for large time jumps
- Add explicit recovery rules for injured and missing people
- Add automated consistency scenarios for travel, war, death, and reload

Exit condition: a person cannot be present in two regions, perform overlapping
hard activities, act after confirmed death, or know an unseen event.

### Milestone 2: one-person daily-life vertical slice

Goal: let the player create an identity and live one coherent day in one city.

- Player identity, gender, appearance seed, background, household, and job
- Causal spawn region derived from identity and household data
- Home, workplace, food, rest, medical, and leisure opportunity providers
- Schedule obligations translated into commitments and opportunities
- Money, hunger, fatigue, health, travel time, and missed-work consequences
- Minimal Blueprint debug screen for current state and decision reasons

Exit condition: the player and several NPCs can complete a full day whose
actions have visible causes and persistent consequences.

### Milestone 3: local social world

Goal: make a small district operate coherently around the player.

- Households, relationships, employers, service providers, and organizations
- Relationship needs, requests, promises, conflict, and message delivery
- Local population instantiation and recycling
- Witness perception, evidence creation, reporting, and basic police response
- Missing-person and violent-crime case records

Exit condition: approximately 50 detailed local NPCs interact without requiring
all city residents to run full behavior logic.

### Milestone 4: scalable city and long-running saves

Goal: prove that long play sessions do not cause unbounded simulation cost.

- Hot/warm/cold entity and event lifecycle
- Aggregate district population, jobs, prices, crime, and service capacity
- Promote/demote people between aggregate and detailed simulation
- SaveGame format, schema version, migration, compaction, and event summaries
- Performance counters and a simulation debug dashboard

Exit condition: simulate at least one in-game year while storage and frame time
remain bounded and contacted NPC history remains consistent.

### Milestone 5: planetary travel

Goal: extend the same consistency rules across regions on one planet.

- Transport graph, routes, timetables, tickets, capacity, delay, and cancellation
- Departed/in-transit/arrived presence intervals
- Alternate routes and missed connections
- Delayed news and relationship consequences

Exit condition: a person can travel between cities without teleportation or
appearing at home during transit.

### Milestone 6: interplanetary and interstellar layer

Goal: reuse the travel model at astronomical scale.

- Planet, orbit, star-system, and route records
- Ships as moving regions containing people and organizations
- Light-speed communication and courier-delivered information versions
- Route hazards, inspections, trade, migration, and stranded populations
- Aggregate simulation for remote planets and societies

Exit condition: travel and news between two planets remain causally consistent
under large time skips.

### Milestone 7: cultivation gameplay vertical slice

Goal: add the playable system without breaking the autonomous world.

- Body, energy, understanding, and control progression
- System scan and opportunity recommendations
- One movement ability, one attack, one defensive action, one active skill
- Training, injury, recovery, material, and energy costs
- Relic incident and the first evolving regional threat

Exit condition: a player can progress through the first cultivation stage while
NPC life and regional events continue independently.

### Milestone 8: civilization and alien escalation

Goal: scale decisions from organizations to civilizations.

- Organization goals, resources, authority, intelligence, and decision cadence
- Technology diffusion and social response
- Day 0 to Day 120 relic pursuit timeline
- Human factions and alien fleet factions making autonomous decisions
- Occupation, resistance, negotiation, capture, and victory continuations

Exit condition: the first fleet crisis evolves with or without player action and
produces no static game-over branch.

### Milestone 9: universe generation and presentation

Goal: expand content breadth only after the simulation kernel is proven.

- Deterministic stars, planets, biospheres, civilizations, and history seeds
- Floating origins and hierarchical coordinate spaces
- Streaming, World Partition, procedural terrain, and visual LOD
- Detailed local actions, animation, item consumption, digestion, and disease
- Art, audio, UI, accessibility, and performance production passes

Exit condition: distant generated worlds can be visited and expanded into local
detail without rewriting their established history.

## 4. Immediate work order

Until the Windows UE 5.8 machine is available, only do work that reduces first
compile risk or supports the smoke test:

1. Add safe read-only query APIs needed by the debug screen.
2. Add a minimal deterministic demo-world bootstrap callable from Blueprint.
3. Add a compact debug snapshot structure for people, activities, commitments,
   truth events, and known messages.
4. Stop adding broad gameplay systems until Milestone 0 compiles successfully.

After UE 5.8 is installed, Milestone 0 takes priority over every other task.

## 5. Non-negotiable rules

- Truth, knowledge, and presentation remain separate.
- Important commitments, deaths, ownership, and history never disappear when
  simulation detail is reduced.
- Distant populations are aggregates, not billions of ticking state machines.
- Every important action has a physical, biological, social, or institutional
  cause and an observable consequence.
- Experimental UE features may improve presentation but cannot own canonical
  simulation data.
- No later milestone begins by bypassing an unverified earlier consistency gate.
