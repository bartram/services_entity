<?php
/**
 * This class exposes entity resources as described by their entity metadata
 * properties.
 *
 * Some of this code is borrowed from restws module.
 */
class ServicesEntityMetadataResourceController extends ServicesEntityResourceController implements ServicesEntityMetadataResourceProcessor, ServicesEntityMetadataResourceFormatter {
  /**
   * The input processor which will be used by this controller.
   * @var \ServicesEntityMetadataResourceProcessor
   */
  public $processor;

  /**
   * The resource formatter which will be used by this controller.
   * @var \ServicesEntityMetadataResourceFormatter
   */
  public $formatter;

  /**
   * A cache of wrappers built for each entity type and set of values.
   * @var array
   */
  protected $wrapperCache;

  /**
   * Constructor.
   */
  public function __construct() {
    $this->processor = $this;
    $this->formatter = $this;
  }

  /**
   * @see ServicesEntityResourceController::access()
   */
  public function access($op, $args) {
    if ($op == 'create') {
      list($entity_type, $data) = $args;
      // Create a wrapper from the entity so we can call its access() method.
      $wrapper = $this->processValues($entity_type, $data, 'create');
      return $wrapper->entityAccess('create');
    }
    else {
      return parent::access($op, $args);
    }
  }

  /**
   * @see ServicesEntityResourceController::create()
   */
  public function create($entity_type, array $values) {
    $wrapper = $this->processor->processValues($entity_type, $values, 'create');
    $wrapper->save();
    // Rewrap to refresn any properties cached in the wrapper.
    $wrapper = entity_metadata_wrapper($entity_type, $wrapper->value());
    return $this->formatter->formatWrapper($wrapper, NULL, 'create');
  }

  /**
   * @see ServicesEntityResourceController::retrieve()
   */
  public function retrieve($entity_type, $entity_id, $fields, $revision) {
    $entity = parent::retrieve($entity_type, $entity_id, '*', $revision);
    $fields = $fields == '*' ? NULL : explode(',', $fields);
    return $this->formatter->formatWrapper(entity_metadata_wrapper($entity_type, $entity), $fields, 'retrieve');
  }

  /**
   * @see ServicesEntityResourceController::update()
   */
  public function update($entity_type, $entity_id, array $values) {
    $wrapper = $this->processor->processValues($entity_type, $values, 'update', $entity_id);
    $wrapper->save();
    // Rewrap to refresn any properties cached in the wrapper.
    $wrapper = entity_metadata_wrapper($entity_type, $wrapper->value());
    return $this->formatter->formatWrapper($wrapper, NULL, 'update');
  }

  /**
   * @see ServicesEntityResourceController::index()
   */
  public function index($entity_type, $fields, $parameters, $page, $pagesize, $sort, $direction) {
    // Call the parent method, which takes care of access control.
    $entities = parent::index($entity_type, '*', $parameters, $page, $pagesize, $sort, $direction);
    $return = array();
    $fields = $fields == '*' ? NULL : explode(',', $fields);
    foreach ($entities as $entity) {
      $return[] = $this->formatter->formatWrapper(entity_metadata_wrapper($entity_type, $entity), $fields, 'index');
    }
    return $return;
  }

  /**
   * Implements ServicesResourceControllerInterface::field().
   */
  public function field($entity_type, $entity_id, $field_name, $fields = '*', $raw = FALSE) {
    $entity = entity_load_single($entity_type, $entity_id);
    if (!$entity) {
      services_error('Entity not found', 404);
    }

    $field_name = preg_replace('/^field_/', '', $field_name);

    // The metadata wrapper checks entity_access() on each entity in the field.
    $return = $this->formatter->formatWrapper(entity_metadata_wrapper($entity_type, $entity), $field_name);
    return $return;
  }

  /**
   * @see ServicesEntityResourceProcessor::processValues()
   *
   * Note: This method returns a cached version of the wrapper, if available.
   * To alter the way the wrapper is created, subclasses should override
   * createWrapperFromValues() instead.
   */
  public function processValues($entity_type, array $values, $op, $entity_id = NULL) {
    // We keep a static cache of the wrappers built for each set of values.
    // This also prevents calling property setters more than once, which is
    // useful for field collections, since they are silly and add the collection
    // to the host entity anew each time you call $wrapper->host_entity->set().
    $cache_key = $entity_type . serialize($values);
    if ($op == 'update') {
      $cache_key .= $entity_id;
    }
    $cache_key = md5($cache_key);

    if (!isset($this->wrapperCache[$cache_key])) {
      $this->wrapperCache[$cache_key] = $this->createWrapperFromValues($entity_type, $values, $op, $entity_id);
    }

    return $this->wrapperCache[$cache_key];
  }

  /**
   * @see ServicesEntityResourceFormatter::formatWrapper()
   */
  public function formatWrapper(\EntityMetadataWrapper $wrapper, array $fields = NULL, $op = 'retrieve') {
    $data = array();
    foreach ($wrapper as $name => $property) {
      // Skip filtered or non-requested properties.
      if ((isset($fields) && !in_array($name, $fields_array)) || !$this->propertyFilter($wrapper, $name, $op)) {
        continue;
      }

      // Attempt to represent this property.
      try {
        if ($property instanceof EntityDrupalWrapper) {
          // For entities only return a reference.
          if ($id = $property->getIdentifier()) {
            $data[$name] = $this->getResourceReference($property->type(), $id);
          }
        }
        elseif ($property instanceof EntityValueWrapper) {
          // If no metadata for this property, simply return it.
          $data[$name] = $property->value();
        }
        elseif ($property instanceof EntityListWrapper || $property instanceof EntityStructureWrapper) {
          // If property has further metadata, then recurse.
          $data[$name] = $this->formatWrapper($property, NULL, $op);
        }
      }
      catch (EntityMetadataWrapperException $e) {
        // A property causes problems - ignore that.
      }
    }
    // If bundle = entity_type, don't send it.
    if (method_exists($wrapper, 'entityInfo')) {
      $entity_info = $wrapper->entityInfo();
      if (isset($entity_info['bundle keys'])) {
        foreach ($entity_info['bundle keys'] as $bundle_key) {
          if (array_key_exists($bundle_key, $data) && $data[$bundle_key] == $wrapper->type()) {
            unset($data[$bundle_key]);
          }
        }
      }
    }
    return $data;
  }

  /**
   * Return a resource reference array.
   *
   * Properties which refer to other resources are represented as references.
   *
   * @param string $resource
   *   The entity type of the resource.
   * @param int $id
   *   The identifier of the resource.
   *
   * @return array
   *   A referene to the resource.
   */
  protected function getResourceReference($resource, $id) {
    $return = array(
      'uri' => services_resource_uri(array('entity_' . $resource, $id)),
      'id' => $id,
      'resource' => $resource,
    );
    if (module_exists('uuid') && entity_get_info($resource)) {
      $ids = entity_get_uuid_by_id($resource, array($id));
      if ($id = reset($ids)) {
        $return['uuid'] = $id;
      }
    }
    return $return;
  }

  /**
   * Overridden to translate metadata property name to schema field.
   *
   * @see ServicesEntityResourceController::propertyQueryOperation()
   */
  protected function propertyQueryOperation($entity_type, EntityFieldQuery $query, $operation, $property, $value) {
    try {
      $field = $this->getSchemaField($entity_type, $property);
      parent::propertyQueryOperation($entity_type, $query, $operation, $field, $value);
    }
    catch (ServicesException $e) {
      // Intercept a services exception and correct the property name.
      services_error(t('Parameter @prop does not exist', array('@p' => $property)), 406);
    }
  }

  /**
   * Helper function to return the schema field for a property.
   *
   * @param string $entity_type
   *   The type of entity to which the property belongs.
   * @param string $property
   *   The property name.
   *
   * @return string
   *   The name of the matching schema field, or the property name of not found.
   */
  protected function getSchemaField($entity_type, $property) {
    $info =& drupal_static(__FUNCTION__);
    if (!isset($info[$entity_type])) {
      // Cache this since Entity API doesn't.
      $info[$entity_type] = entity_get_all_property_info($entity_type);
    }
    return isset($info[$entity_type][$property]['schema field']) ? $info[$entity_type][$property]['schema field'] : $property;
  }

  /**
   * Create a wrapped entity from provided data values.
   *
   * This method contains the implementation logic for processValues().
   * Subclasses may override this method to alter the way input values are
   * preprocessed before creating a wrapper.
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
   *   A wrapper built from the supplied values.
   */
  protected function createWrapperFromValues($entity_type, array $values, $op, $entity_id = NULL) {
    try {
      if (isset($entity_id)) {
        // For update, we create a wrapper from the original entity.
        $wrapper = entity_metadata_wrapper($entity_type, $entity_id);
      }
      else {
        // For create, we build a new wrapper using only the entity keys.
        // Note: we cannot use entity_property_values_create_entity()
        // because we want to apply property filters and access callbacks
        // before setting non 'entity key' properties.
        $entity_info = entity_get_info($entity_type);
        $entity_keys = array_flip(array_filter($entity_info['entity keys']));
        $creation_values = array();
        foreach ($values as $name => $value) {
          $key = $this->getSchemaField($entity_type, $name);
          if (isset($entity_keys[$key])) {
            // Don't attempt to set the primary key.
            if (!isset($entity_keys['id']) || ($key != $entity_keys['id'])) {
              $creation_values[$key] = $value;
            }
            // Remove this from the list of properties to be set. This means
            // we do not enforce property level access for entity keys. We
            // assume that if you have permission to create an entity, you
            // must also have permission to initialize its required keys.
            unset($values[$name]);
          }
        }

        // If an entity supports only a single bundle, then use that as a
        // default. This allows creation of such entities if (as with ECK)
        // they still use a bundle key.
        if (isset($entity_keys['bundle'])
            && empty($creation_values[$entity_keys['bundle']])
            && isset($entity_info['bundles'])
            && count($entity_info['bundles']) === 1) {
          $creation_values[$entity_keys['bundle']] = reset($entity_info['bundles']);
        }

        $entity = entity_create($entity_type, $creation_values);
        $wrapper = entity_metadata_wrapper($entity_type, $entity);
      }

      // Now loop through all remaining properties.
      foreach ($values as $name => $value) {
        // Handle Resource references.
        if (is_array($value) && isset($value['id'])) {
          if (isset($value['resource'])) {
            // If the resource type is available, use a wrapper. This allows
            // us to set properties of the generic 'entity' type - for example
            // the field_collection_item's host_entity property.
            $value = entity_metadata_wrapper($value['resource'], $value['id']);
          }
          else {
            $value = $value['id'];
          }
        }
        if ($this->propertyFilter($wrapper, $name, $op, $value)) {
          $wrapper->$name->set($value);
          if (!$this->propertyAccess($wrapper, $name, $op)) {
            services_error(t("Not authorized to set property '@property-name'.", array('@property-name' => $name)), 401);
          }
        }
      }
    }
    catch (EntityMetadataWrapperException $e) {
      services_error($e->getMessage(), 406);
    }
    return $wrapper;
  }

  /**
   * Determine whether a given property should be included or ignored.
   *
   * For read operations ('retrieve' and 'index') this method determines whether
   * a property should be included in the result. For write operations
   * ('create' and 'update') it determines whether an attempt should be
   * made to set the property.
   *
   * Note that unlike the propertyAccess() method, a FALSE return value
   * from this method will not generally raise an exception.
   *
   * @param \EntityMetadataWrapper $wrapper
   *   The wrapper whose property is to be checked.
   * @param $name
   *   The name of the property to check.
   * @param string $op
   *   One of 'create', 'update' or 'retrieve'
   * @param mixed $value
   *   For write operations, the value to which the property is being set.
   *
   * @return boolean
   *   TRUE if the given property should be included. FALSE otherwise.
   */
  protected function propertyFilter($wrapper, $name, $op, $value = NULL) {
    switch ($op) {
      case 'update':
        // Only attempt to set properties when the new value differs from that
        // on the existing entity; otherwise, requests will fail for read-only
        // or unauthorized properties, even if they are not being changed. This
        // allows us to UPDATE a previously retrieved entity without removing
        // such properties from the payload, as long as they are unchanged.
        $property = $wrapper->{$name};
        if ($property instanceof EntityDrupalWrapper) {
          if ($value instanceof EntityDrupalWrapper) {
            return $value->getIdentifier() != $property->getIdentifier();
          }
          elseif (is_numeric($value)) {
            return $value != $property->getIdentifier();
          }
        }
        return $value != $property->value();

      case 'retrieve':
      case 'index':
        // For retrieve operations, we simply filter out properties to which
        // the user does not have access.
        return $this->propertyAccess($wrapper, $name, $op);

      case 'create':
        // No
        return TRUE;
    }
  }

  /**
   * Check access on an entity metadata property.
   *
   * This is a wrapper around EntityMetadataWrapper::access() to compensate
   * for several shortcomings of that method.
   *
   * @param EntityDrupalWrapper $wrapper
   *   The wrapped entity for which the property access is to be checked.
   * @param string $name
   *   The wrapper name of the property whose access is to be checked.
   * @param string $op
   *   One of 'create', 'update', 'retrieve' or 'index'.
   *
   * @return bool
   *   TRUE if the current user has access to set the property, FALSE otherwise.
   */
  protected function propertyAccess($wrapper, $name, $op) {
    $property = $wrapper->{$name};
    try {
      switch ($op) {
        case 'create':
          // Don't check access on node author if set to the current user.
          if ($wrapper->type() == 'node' && $name == 'author' && $wrapper->value()->uid == $GLOBALS['user']->uid) {
            return TRUE;
          }

          // No break: no special cases apply, so contine as for 'update'.

        case 'update':
          // This is a hack to check format access for text fields.
          // @todo remove once this is handled properly by core or Entity API.
          // @see https://drupal.org/node/2065021
          if ($property->type() == 'text_formatted' && $property->format->value()) {
            $format = (object) array('format' => $property->format->value());
            if (!filter_access($format)) {
              return FALSE;
            }
          }

          // Entity API propertyAccess() will throw notices for nodes with no
          // nid, because it does not distinguish betweeb 'create' and 'update'
          // operations, and always calls entity_metadata_no_hook_node_access()
          // with the 'update' op, which assumes that a node has a nid.
          // @todo remove once this is fixed in Entity API.
          // @see https://drupal.org/node/2203961
          if ($wrapper->type() == 'node' && !$wrapper->getIdentifier()) {
            return TRUE;
          }

          // Finally, use the property access.
          return $property->access('edit');

        case 'retrieve':
        case 'index':
            return $property->access('view');
      }
    }
    catch (EntityMetaDataWrapperException $e) {
      // Log the exception and ignore the property. This is known to happen
      // when attempting to access the 'book' property of a non-book node.
      // In such cases Entity API erroneously throws an exception.
      // @see https://drupal.org/node/2051087 and linked issues.
      watchdog('services_entity', 'Exception testing access to property @p: @e', array('@p' => $name, '@e' => $e->getMessage()), WATCHDOG_WARNING);
      return FALSE;
    }
  }
}
