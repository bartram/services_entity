<?php
/**
 * Defines methods used to process input values.
 */
interface ServicesEntityMetadataResourceProcessor {
  /**
   * Process input values into a wrapped entity.
   *
   * The wrapper returned by this method should have all filters and access
   * callbacks applied, and be ready for saving.
   *
   * @param $entity_type
   *   The type of entity for which a wrapper is to be created.
   * @param $values
   *   Array of data property values.
   * @param $op
   *   Either 'update' or 'create'.
   * @param $entity_id
   *   For update operations, the entity being updated.
   *
   * @return \EntityDrupalWrapper
   *   A wrapped entity built from the supplied values.
   */
  function processValues($entity_type, array $values, $op, $entity_id = NULL);
}
