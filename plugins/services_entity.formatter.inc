<?php
/**
 * Defines methods used to format wrapped data as a resource.
 */
interface ServicesEntityMetadataResourceFormatter {
  /**
   * Return a representation of the wrapped entity.
   *
   * This method takes a wrapped entity or property, and converts it to an
   * associative array mapping property names to values, using the entity
   * metadata API.
   *
   * @param \EntityMetadataWrapper $wrapper
   *   A wrapper to be represented.
   * @param array $fields
   *   A list of fields to include, or NULL to include all fields.
   * @param $op
   *   The operation for which the representation is to be generated.
   *
   * @return array
   *   An associative array of data values, keyed by property name.
   */
  function formatWrapper(\EntityMetadataWrapper $wrapper, array $fields = NULL, $op = 'retrieve');
}